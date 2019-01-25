/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "TrustyKeymaster"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include <type_traits>
#include <algorithm>
#include <hardware/keymaster2.h>

#include <keymaster/authorization_set.h>
#include <log/log.h>

#include "trusty_keymaster2_device.h"
#include "trusty_keymaster_ipc.h"
#include "keymaster_ipc.h"

const uint32_t SEND_BUF_SIZE = 8192;
const uint32_t RECV_BUF_SIZE = 8192;
/* The sizeof buf is 4k, but the size included header of message,
 * So the max size of data is 4040.
*/
/*lint -e681*/
const uint32_t ROUND_BUF_SIZE = (4*1010);
namespace keymaster {

const size_t kMaximumAttestationChallengeLength = 128;

static keymaster_error_t translate_error(int err) {
    switch (err) {
    case 0:
        return KM_ERROR_OK;
    case -EPERM:
    case -EACCES:
        return KM_ERROR_SECURE_HW_ACCESS_DENIED;

    case -ECANCELED:
        return KM_ERROR_OPERATION_CANCELLED;

    case -ENODEV:
        return KM_ERROR_UNIMPLEMENTED;

    case -ENOMEM:
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    case -EBUSY:
        return KM_ERROR_SECURE_HW_BUSY;

    case -EIO:
        return KM_ERROR_SECURE_HW_COMMUNICATION_FAILED;

    case -EMSGSIZE:
    case -EOVERFLOW:
        return KM_ERROR_INVALID_INPUT_LENGTH;

    default:
        return KM_ERROR_UNKNOWN_ERROR;
    }
}

template <typename RequestType>
static void AddClientAndAppData(const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
                         RequestType* request);
struct KeyParamSetContents_Delete {
    void operator()(keymaster_key_param_set_t* p) { keymaster_free_param_set(p); }
};
template <typename RequestType>
static void AddClientAndAppData(const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
                         RequestType* request) {
    request->additional_params.Clear();
    if (client_id)
        request->additional_params.push_back(TAG_APPLICATION_ID, *client_id);
    if (app_data)
        request->additional_params.push_back(TAG_APPLICATION_DATA, *app_data);
}

bool TrustyKeymasterDevice::configured_ = false;

TrustyKeymasterDevice::TrustyKeymasterDevice(const hw_module_t* module) {
    static_assert(std::is_standard_layout<TrustyKeymasterDevice>::value,
                  "TrustyKeymasterDevice must be standard layout");
    static_assert(offsetof(TrustyKeymasterDevice, device_) == 0,
                  "device_ must be the first member of KeymasterOpenSsl");
    static_assert(offsetof(TrustyKeymasterDevice, device_.common) == 0,
                  "common must be the first member of keymaster_device");

    ALOGI("Creating device");
    ALOGD("Device address: %p", this);

    memset(&device_, 0, sizeof(device_));

    device_.common.tag = HARDWARE_DEVICE_TAG;
    device_.common.version = 1;
    device_.common.module = const_cast<hw_module_t*>(module);
    device_.common.close = close_device;

    device_.flags = KEYMASTER_BLOBS_ARE_STANDALONE | KEYMASTER_SUPPORTS_EC;


    device_.configure=configure;
    device_.upgrade_key=upgrade_key;
    device_.attest_key=attest_key;
    device_.add_rng_entropy = add_rng_entropy;
    device_.generate_key = generate_key;
    device_.get_key_characteristics = get_key_characteristics;
    device_.import_key = import_key;
    device_.export_key = export_key;
    device_.delete_key = delete_key;
    device_.delete_all_keys = delete_all_keys;
    device_.begin = begin;
    device_.update = update;
    device_.finish = finish;
    device_.abort = abort;

    device_.context = NULL;

    int rc = trusty_keymaster_connect();
    error_ = translate_error(rc);
    if (rc < 0) {
        ALOGE("failed to connect to keymaster (%d)", rc);
        return;
    }

    GetVersionRequest version_request;
    GetVersionResponse version_response;
    error_ = Send(version_request, &version_response);
    if (error_ == KM_ERROR_INVALID_ARGUMENT || error_ == KM_ERROR_UNIMPLEMENTED) {
        ALOGI("\"Bad parameters\" error on GetVersion call.  Assuming version 0.");
        message_version_ = 0;
        error_ = KM_ERROR_OK;
    }
    message_version_ = MessageVersion(version_response.major_ver, version_response.minor_ver,
                                      version_response.subminor_ver);
    if (message_version_ < 0) {
        // Can't translate version?  Keymaster implementation must be newer.
        ALOGE("Keymaster version %d.%d.%d not supported.", version_response.major_ver,
              version_response.minor_ver, version_response.subminor_ver);
        error_ = KM_ERROR_VERSION_MISMATCH;
    }
}

TrustyKeymasterDevice::~TrustyKeymasterDevice() {
    trusty_keymaster_disconnect();
}

hw_device_t* TrustyKeymasterDevice::hw_device() {
    return &device_.common;
}

/* static */
int TrustyKeymasterDevice::close_device(hw_device_t* dev) {
    delete reinterpret_cast<TrustyKeymasterDevice*>(dev);
    return 0;
}

keymaster_key_characteristics_t* BuildCharacteristics(const AuthorizationSet& hw_enforced,
                                                      const AuthorizationSet& sw_enforced) {
    keymaster_key_characteristics_t* characteristics =
        reinterpret_cast<keymaster_key_characteristics_t*>(
            malloc(sizeof(keymaster_key_characteristics_t)));
    if (characteristics) {
        hw_enforced.CopyToParamSet(&characteristics->hw_enforced);
        sw_enforced.CopyToParamSet(&characteristics->sw_enforced);
    }
    return characteristics;
}

keymaster_error_t TrustyKeymasterDevice::configure(const keymaster2_device_t* dev,
                                   const keymaster_key_param_set_t* params){
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    AuthorizationSet params_copy(*params);
    uint32_t os_version;
    uint32_t os_patchlevel;
    if (!params_copy.GetTagValue(TAG_OS_VERSION, &os_version) ||
        !params_copy.GetTagValue(TAG_OS_PATCHLEVEL, &os_patchlevel)) {
        ALOGE("Configuration parameters must contain OS version and patch level");
        return KM_ERROR_INVALID_ARGUMENT;
    }
    ConfigureResponse response;
    ConfigureRequest request;
    request.os_patchlevel = os_patchlevel;
    request.os_version= os_version;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }
    if (response.error == KM_ERROR_OK){
        TrustyKeymasterDevice::configured_ = true;
    }
    return err;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::add_rng_entropy(const keymaster2_device_t* dev,
                                                       const uint8_t* data, size_t data_length) {
    if (!dev ||!data)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    AddEntropyRequest request;
    request.random_data.Reinitialize(data, (data_length > ROUND_BUF_SIZE)?ROUND_BUF_SIZE:data_length);
    AddEntropyResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        printf("add_rng_entropy failed with %d", response.error);
    return response.error;
}

keymaster_error_t TrustyKeymasterDevice::generate_key(const keymaster2_device_t* dev,
                                          const keymaster_key_param_set_t* params,
                                          keymaster_key_blob_t* key_blob,
                                          keymaster_key_characteristics_t* characteristics){
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    GenerateKeyRequest request;
    GenerateKeyResponse response;
    request.key_description.Reinitialize(*params);

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    key_blob->key_material_size = response.key_blob.key_material_size;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.key_blob.key_material, response.key_blob.key_material_size);
    key_blob->key_material = tmp;

    if (characteristics) {
        response.enforced.CopyToParamSet(&characteristics->hw_enforced);
        response.unenforced.CopyToParamSet(&characteristics->sw_enforced);
    }

    return KM_ERROR_OK;
}
keymaster_error_t TrustyKeymasterDevice::get_key_characteristics(const keymaster2_device_t* dev,
                                                     const keymaster_key_blob_t* key_blob,
                                                     const keymaster_blob_t* client_id,
                                                     const keymaster_blob_t* app_data,
                                                     keymaster_key_characteristics_t* characteristics){
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!characteristics)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    GetKeyCharacteristicsRequest request;
    request.SetKeyMaterial(*key_blob);
    AddClientAndAppData(client_id, app_data, &request);

    GetKeyCharacteristicsResponse response;
    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    response.enforced.CopyToParamSet(&characteristics->hw_enforced);
    response.unenforced.CopyToParamSet(&characteristics->sw_enforced);

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::import_key(
    const keymaster_key_param_set_t* params,
    keymaster_key_format_t key_format, const keymaster_blob_t* key_data,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t** characteristics) {
    if (!params || !key_data)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    ImportKeyRequest request;
    request.key_description.Reinitialize(*params);

    request.key_format = key_format;
    request.SetKeyMaterial(key_data->data, key_data->data_length);

    ImportKeyResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    key_blob->key_material_size = response.key_blob.key_material_size;
    key_blob->key_material = reinterpret_cast<uint8_t*>(malloc(key_blob->key_material_size));
    if (!key_blob->key_material)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(const_cast<uint8_t*>(key_blob->key_material), response.key_blob.key_material,
           response.key_blob.key_material_size);

    if (characteristics) {
        *characteristics = BuildCharacteristics(response.enforced, response.unenforced);
        if (!*characteristics)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    return KM_ERROR_OK;
}

keymaster_error_t TrustyKeymasterDevice::import_key(const keymaster2_device_t* dev,
                                        const keymaster_key_param_set_t* params,
                                        keymaster_key_format_t key_format,
                                        const keymaster_blob_t* key_data,
                                        keymaster_key_blob_t* key_blob,
                                        keymaster_key_characteristics_t* characteristics){
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!params || !key_data)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;
    keymaster_error_t error = KM_ERROR_OK;
    if (characteristics) {
        keymaster_key_characteristics_t *characteristics_ptr =
            (keymaster_key_characteristics_t*)malloc(sizeof(keymaster_key_characteristics_t));
        error = import_key(params, key_format, key_data, key_blob,&characteristics_ptr);
        if (error == KM_ERROR_OK) {
            *characteristics = *characteristics_ptr;
            free(characteristics_ptr);
        }
    } else {
        error = import_key(params, key_format, key_data, key_blob, nullptr);
    }

    return error;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::export_key(const keymaster2_device_t* dev,
                                                  keymaster_key_format_t export_format,
                                                  const keymaster_key_blob_t* key_to_export,
                                                  const keymaster_blob_t* client_id,
                                                  const keymaster_blob_t* app_data,
                                                  keymaster_blob_t* export_data) {
    if (!dev || !key_to_export || !key_to_export->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!export_data)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;


    export_data->data = nullptr;
    export_data->data_length = 0;

    ExportKeyRequest request;
    request.key_format = export_format;
    request.SetKeyMaterial(*key_to_export);
    AddClientAndAppData(client_id, app_data, &request);

    ExportKeyResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    export_data->data_length = response.key_data_length;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(export_data->data_length));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.key_data, export_data->data_length);
    export_data->data = tmp;
    return KM_ERROR_OK;
}
keymaster_error_t TrustyKeymasterDevice::attest_key(const keymaster2_device_t* dev,
                                    const keymaster_key_blob_t* key_to_attest,
                                    const keymaster_key_param_set_t* attest_params,
                                    keymaster_cert_chain_t* cert_chain){
    if (!dev || !key_to_attest || !attest_params || !cert_chain)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    cert_chain->entry_count = 0;
    cert_chain->entries = nullptr;

    AttestKeyRequest request;
    request.SetKeyMaterial(*key_to_attest);
    request.attest_params.Reinitialize(*attest_params);

    keymaster_blob_t attestation_challenge = {};
    request.attest_params.GetTagValue(TAG_ATTESTATION_CHALLENGE, &attestation_challenge);
    if (attestation_challenge.data_length > kMaximumAttestationChallengeLength) {
        ALOGE("%d-byte attestation challenge; only %d bytes allowed",
              attestation_challenge.data_length, kMaximumAttestationChallengeLength);
        return KM_ERROR_INVALID_INPUT_LENGTH;
    }

    AttestKeyResponse response;
    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    // Allocate and clear storage for cert_chain.
    keymaster_cert_chain_t& rsp_chain = response.certificate_chain;
    cert_chain->entries = reinterpret_cast<keymaster_blob_t*>(
        malloc(rsp_chain.entry_count * sizeof(*cert_chain->entries)));
    if (!cert_chain->entries)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    cert_chain->entry_count = rsp_chain.entry_count;
    for (keymaster_blob_t& entry : array_range(cert_chain->entries, cert_chain->entry_count))
        entry = {};

    // Copy cert_chain contents
    size_t i = 0;
    for (keymaster_blob_t& entry : array_range(rsp_chain.entries, rsp_chain.entry_count)) {
        cert_chain->entries[i].data = reinterpret_cast<uint8_t*>(malloc(entry.data_length));
        if (!cert_chain->entries[i].data) {
            keymaster_free_cert_chain(cert_chain);
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        }
        cert_chain->entries[i].data_length = entry.data_length;
        memcpy(const_cast<uint8_t*>(cert_chain->entries[i].data), entry.data, entry.data_length);
        ++i;
    }

    return KM_ERROR_OK;
}

keymaster_error_t TrustyKeymasterDevice::upgrade_key(const keymaster2_device_t* dev,
                                     const keymaster_key_blob_t* key_to_upgrade,
                                     const keymaster_key_param_set_t* upgrade_params,
                                     keymaster_key_blob_t* upgraded_key){
    if (!dev || !key_to_upgrade || !upgrade_params)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!upgraded_key)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    UpgradeKeyRequest request;
    request.SetKeyMaterial(*key_to_upgrade);
    request.upgrade_params.Reinitialize(*upgrade_params);

    UpgradeKeyResponse response;
    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    upgraded_key->key_material_size = response.upgraded_key.key_material_size;
    uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(upgraded_key->key_material_size));
    if (!tmp)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    memcpy(tmp, response.upgraded_key.key_material, response.upgraded_key.key_material_size);
    upgraded_key->key_material = tmp;

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::delete_key(const keymaster2_device_t* dev,
                                                  const keymaster_key_blob_t* key) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    // we have nothing to delete
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::delete_all_keys(const keymaster2_device_t* dev) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    // we have nothing to delete
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::begin(const keymaster2_device_t* dev,
                                             keymaster_purpose_t purpose,
                                             const keymaster_key_blob_t* key,
                                             const keymaster_key_param_set_t* in_params,
                                             keymaster_key_param_set_t* out_params,
                                             keymaster_operation_handle_t* operation_handle) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!operation_handle)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }

    BeginOperationRequest request;
    request.purpose = purpose;
    request.SetKeyMaterial(*key);
    if(in_params)
        request.additional_params.Reinitialize(*in_params);

    BeginOperationResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    *operation_handle = response.op_handle;
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::update(const keymaster2_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* in_params,
                                              const keymaster_blob_t* input, size_t* input_consumed,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {

    if (!dev || !input)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!input_consumed)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }
    if (output) {
        output->data = nullptr;
        output->data_length = 0;
    }

    uint32_t round = 0;
    uint32_t left = 0;
    uint8_t *tmp = NULL;
    if (input) {
        round = input->data_length/ROUND_BUF_SIZE;
        left = input->data_length%ROUND_BUF_SIZE;
        if(left > 0) {
            round++;
        }
        ALOGE("jacky, we have %d rounds, left=%d, total=%d", round, left, input->data_length);
        *input_consumed = 0;
        if(round > 0 && output) {
            tmp = reinterpret_cast<uint8_t*>(malloc(input->data_length+ROUND_BUF_SIZE));
            if (!tmp)
                return KM_ERROR_MEMORY_ALLOCATION_FAILED;
            output->data = tmp;
        }
    }

    //here for AES GCM aad update
    if(0 == round) {
        UpdateOperationRequest request;
        request.op_handle = operation_handle;
        request.input.Reinitialize(input->data, 0);
        if (in_params )
            request.additional_params.Reinitialize(*in_params);

        UpdateOperationResponse response;

        keymaster_error_t err = Send(request, &response);

        if (err != KM_ERROR_OK) {
            ALOGE("jacky,round=0 we send done, receive error=%d", err);
            ALOGE("Got error %d from send", err);
            return err;
        }

        if (response.error != KM_ERROR_OK) {
            ALOGE("jacky,round=0 we check resp done, receive error=%d", err);
            return response.error;
        }
    }

    for(uint32_t i=0; i < round; i++) {
        //ALOGE("jacky, we are updating round %d/%d left=%d", i, round, left);
        UpdateOperationRequest request;
        request.op_handle = operation_handle;
        uint32_t data_len = (i==round-1 && left > 0)? left:ROUND_BUF_SIZE;
        request.input.Reinitialize(input->data + ROUND_BUF_SIZE*i, data_len);
        if (in_params && (0==i))
            request.additional_params.Reinitialize(*in_params);

        UpdateOperationResponse response;

        keymaster_error_t err = Send(request, &response);
        if (err != KM_ERROR_OK) {
            ALOGE("jacky, we send done, receive error=%d", err);
            ALOGE("Got error %d from send", err);
            if(output) {
                free(tmp);
                output->data = NULL;
                output->data_length = 0;
            }
            return err;
        }

        if (response.error != KM_ERROR_OK) {
            ALOGE("jacky, we check resp done, receive error=%d", err);
            if(output) {
                free(tmp);
                output->data = NULL;
                output->data_length = 0;
            }
            return response.error;
        }

        if (response.output_params.size() > 0) {
            if (out_params)
                response.output_params.CopyToParamSet(out_params);
            else {
                if(output) {
                    free(tmp);
                    output->data = NULL;
                    output->data_length = 0;
                }
                return KM_ERROR_OUTPUT_PARAMETER_NULL;
            }
        }

        *input_consumed += response.input_consumed;
        if (output) {
            memcpy(tmp+output->data_length, response.output.peek_read(), response.output.available_read());
            output->data_length += response.output.available_read();
        } else if (response.output.available_read() > 0) {
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
        }
    }
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::finish(const keymaster2_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* params,
                                              const keymaster_blob_t* signature,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (out_params) {
        out_params->params = nullptr;
        out_params->length = 0;
    }

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (output) {
        output->data = nullptr;
        output->data_length = 0;
    }

    FinishOperationRequest request;
    request.op_handle = operation_handle;
    if (signature && signature->data_length > 0)
        request.signature.Reinitialize(signature->data, signature->data_length);
    if(params)
        request.additional_params.Reinitialize(*params);

    FinishOperationResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }
    if (output) {
        output->data_length = response.output.available_read();
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
        if (!tmp)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        memcpy(tmp, response.output.peek_read(), output->data_length);
        output->data = tmp;
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    return KM_ERROR_OK;
}

keymaster_error_t TrustyKeymasterDevice::finish(const keymaster2_device_t* dev,
                                keymaster_operation_handle_t operation_handle,
                                const keymaster_key_param_set_t* in_params,
                                const keymaster_blob_t* input, const keymaster_blob_t* signature,
                                keymaster_key_param_set_t* out_params, keymaster_blob_t* output){
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    if (out_params)
        *out_params = {};

    if (output)
        *output = {};

    if (operation_handle) {
        // This operation is being handled by km1_dev (or doesn't exist).  Pass it through to
        // km1_dev.  Otherwise, we'll use the software AndroidKeymaster, which may delegate to
        // km1_dev after doing necessary digesting.

        std::vector<uint8_t> accumulated_output;
        AuthorizationSet accumulated_out_params;
        AuthorizationSet mutable_params(*in_params);
        if (input && input->data && input->data_length) {
            // Keymaster1 doesn't support input to finish().  Call update() to process input.

            accumulated_output.reserve(input->data_length);  // Guess at output size
            keymaster_blob_t mutable_input = *input;

            while (mutable_input.data_length > 0) {
                keymaster_key_param_set_t update_out_params = {};
                keymaster_blob_t update_output = {};
                size_t input_consumed = 0;
                keymaster_error_t error =
                    update(dev, operation_handle, &mutable_params, &mutable_input,
                                    &input_consumed, &update_out_params, &update_output);
                if (error != KM_ERROR_OK) {
                    return error;
                }

                accumulated_output.reserve(accumulated_output.size() + update_output.data_length);
                std::copy(update_output.data, update_output.data + update_output.data_length,
                          std::back_inserter(accumulated_output));
                free(const_cast<uint8_t*>(update_output.data));

                accumulated_out_params.push_back(update_out_params);
                keymaster_free_param_set(&update_out_params);

                mutable_input.data += input_consumed;
                mutable_input.data_length -= input_consumed;

                // AAD should only be sent once, so remove it if present.
                int aad_pos = mutable_params.find(TAG_ASSOCIATED_DATA);
                if (aad_pos != -1) {
                    mutable_params.erase(aad_pos);
                }

                if (input_consumed == 0) {
                    // Apparently we need more input than we have to complete an operation.
                    abort(dev, operation_handle);
                    return KM_ERROR_INVALID_INPUT_LENGTH;
                }
            }
        }

        keymaster_key_param_set_t finish_out_params = {};
        keymaster_blob_t finish_output = {};
        keymaster_error_t error = finish(dev, operation_handle, &mutable_params,
                                                  signature, &finish_out_params, &finish_output);
        if (error != KM_ERROR_OK) {
            return error;
        }

        if (!accumulated_out_params.empty()) {
            accumulated_out_params.push_back(finish_out_params);
            keymaster_free_param_set(&finish_out_params);
            accumulated_out_params.Deduplicate();
            accumulated_out_params.CopyToParamSet(&finish_out_params);
        }
        std::unique_ptr<keymaster_key_param_set_t, KeyParamSetContents_Delete>
            finish_out_params_deleter(&finish_out_params);

        if (!accumulated_output.empty()) {
            size_t finish_out_length = accumulated_output.size() + finish_output.data_length;
            uint8_t* finish_out_buf = reinterpret_cast<uint8_t*>(malloc(finish_out_length));

            std::copy(accumulated_output.begin(), accumulated_output.end(), finish_out_buf);
            std::copy(finish_output.data, finish_output.data + finish_output.data_length,
                      finish_out_buf + accumulated_output.size());

            free(const_cast<uint8_t*>(finish_output.data));
            finish_output.data_length = finish_out_length;
            finish_output.data = finish_out_buf;
        }
        std::unique_ptr<uint8_t, Malloc_Delete> finish_output_deleter(
            const_cast<uint8_t*>(finish_output.data));

        if ((!out_params && finish_out_params.length) || (!output && finish_output.data_length)) {
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
        }

        if (out_params) {
            *out_params = finish_out_params;
        }

        if (output) {
            *output = finish_output;
        }

        finish_out_params_deleter.release();
        finish_output_deleter.release();

        return KM_ERROR_OK;
    }

    FinishOperationRequest request;
    request.op_handle = operation_handle;
    if (signature && signature->data_length > 0)
        request.signature.Reinitialize(signature->data, signature->data_length);
    if (input && input->data_length > 0)
        request.input.Reinitialize(input->data, input->data_length);
    request.additional_params.Reinitialize(*in_params);

    FinishOperationResponse response;
    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    if (response.output_params.size() > 0) {
        if (out_params)
            response.output_params.CopyToParamSet(out_params);
        else
            return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }
    if (output) {
        output->data_length = response.output.available_read();
        uint8_t* tmp = reinterpret_cast<uint8_t*>(malloc(output->data_length));
        if (!tmp)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        memcpy(tmp, response.output.peek_read(), output->data_length);
        output->data = tmp;
    } else if (response.output.available_read() > 0) {
        return KM_ERROR_OUTPUT_PARAMETER_NULL;
    }

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::abort(const keymaster2_device_t* dev,
                                             keymaster_operation_handle_t operation_handle) {

    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;
    if (!TrustyKeymasterDevice::configured_)
        return KM_ERROR_KEYMASTER_NOT_CONFIGURED;

    AbortOperationRequest request;
    request.op_handle = operation_handle;
    AbortOperationResponse response;
    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }
    return response.error;
}

keymaster_error_t TrustyKeymasterDevice::Send(uint32_t command, const Serializable& req,
                                              KeymasterResponse* rsp) {
    uint32_t req_size = req.SerializedSize();
    if (req_size > SEND_BUF_SIZE)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    uint8_t send_buf[SEND_BUF_SIZE];
    Eraser send_buf_eraser(send_buf, SEND_BUF_SIZE);
    req.Serialize(send_buf, send_buf + req_size);

    // Send it
    uint8_t recv_buf[RECV_BUF_SIZE];
    Eraser recv_buf_eraser(recv_buf, RECV_BUF_SIZE);
    uint32_t rsp_size = RECV_BUF_SIZE;

    int rc = trusty_keymaster_call(command, send_buf, req_size, recv_buf, &rsp_size);
    if (rc < 0) {
        ALOGE("tipc error: %d\n", rc);
        // TODO(swillden): Distinguish permanent from transient errors and set error_ appropriately.
        return translate_error(rc);
    } else {
        ALOGV("Received %d byte response\n", rsp_size);
    }

    const keymaster_message* msg = (keymaster_message *) recv_buf;
    const uint8_t *p = msg->payload;
    if (!rsp->Deserialize(&p, p + rsp_size)) {
        ALOGE("Error deserializing response of size %d\n", (int)rsp_size);
        return KM_ERROR_UNKNOWN_ERROR;
    } else if (rsp->error != KM_ERROR_OK) {
        ALOGE("Response of size %d contained error code %d\n", (int)rsp_size, (int)rsp->error);
        return rsp->error;
    }
    return rsp->error;
}
}  // namespace keymaster
