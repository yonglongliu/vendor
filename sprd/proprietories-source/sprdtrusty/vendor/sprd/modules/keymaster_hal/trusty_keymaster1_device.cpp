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

#include "trusty_keymaster1_device.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>

#include <type_traits>

#include <algorithm>

//#include <openssl/evp.h>
//#include <openssl/x509.h>

#define LOG_TAG "TrustyKeymaster"
#include <cutils/log.h>
#include <hardware/keymaster1.h>

#include <keymaster/authorization_set.h>

#include "trusty_keymaster_ipc.h"
#include "keymaster_ipc.h"

const uint32_t SEND_BUF_SIZE = 8192;
const uint32_t RECV_BUF_SIZE = 8192;
/* The sizeof buf is 4k, but the size included header of message,
 * So the max size of data is 4040.
*/
const uint32_t ROUND_BUF_SIZE = (4*1010);


namespace keymaster {

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

TrustyKeymasterDevice::TrustyKeymasterDevice(const hw_module_t* module) {
    static_assert(std::is_standard_layout<TrustyKeymasterDevice>::value,
                  "TrustyKeymasterDevice must be standard layout");
    static_assert(offsetof(TrustyKeymasterDevice, device_) == 0,
                  "device_ must be the first member of KeymasterOpenSsl");
    static_assert(offsetof(TrustyKeymasterDevice, device_.common) == 0,
                  "common must be the first member of keymaster_device");

    ALOGI("Creating device");
    ALOGI("Creating device2");
    ALOGD("Device address: %p", this);

    memset(&device_, 0, sizeof(device_));

    device_.common.tag = HARDWARE_DEVICE_TAG;
    device_.common.version = 1;
    device_.common.module = const_cast<hw_module_t*>(module);
    device_.common.close = close_device;

    device_.flags = KEYMASTER_BLOBS_ARE_STANDALONE | KEYMASTER_SUPPORTS_EC;

    device_.generate_keypair = NULL;
    device_.import_keypair = NULL;
    device_.get_keypair_public = NULL;
    device_.delete_keypair = NULL;
    device_.delete_all = NULL;
    device_.sign_data = NULL;
    device_.verify_data = NULL;
    device_.get_supported_algorithms = get_supported_algorithms;
    device_.get_supported_block_modes = get_supported_block_modes;
    device_.get_supported_padding_modes = get_supported_padding_modes;
    device_.get_supported_digests = get_supported_digests;
    device_.get_supported_import_formats = get_supported_import_formats;
    device_.get_supported_export_formats = get_supported_export_formats;
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
    /*
     * We should call configure here to init the trust of root, just implement for keymaster1.0.
    */
    configure();
}

TrustyKeymasterDevice::~TrustyKeymasterDevice() {
    trusty_keymaster_disconnect();
}

keymaster_error_t TrustyKeymasterDevice::get_supported_algorithms(const keymaster1_device_t* dev,
                                                                keymaster_algorithm_t** algorithms,
                                                                size_t* algorithms_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!algorithms || !algorithms_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SupportedAlgorithmsRequest request;
    SupportedAlgorithmsResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK) {
        printf("get_supported_algorithms failed with %d", response.error);

        return response.error;
    }

    *algorithms_length = response.results_length;
    *algorithms =
        reinterpret_cast<keymaster_algorithm_t*>(malloc(*algorithms_length * sizeof(**algorithms)));
    if (!*algorithms)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *algorithms);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::get_supported_block_modes(const keymaster1_device_t* dev,
                                                                 keymaster_algorithm_t algorithm,
                                                                 keymaster_purpose_t purpose,
                                                                 keymaster_block_mode_t** modes,
                                                                 size_t* modes_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!modes || !modes_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SupportedBlockModesRequest request;
    request.algorithm = algorithm;
    request.purpose = purpose;
    SupportedBlockModesResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }


    if (response.error != KM_ERROR_OK) {
        printf("get_supported_block_modes failed with %d", response.error);

        return response.error;
    }

    *modes_length = response.results_length;
    *modes = reinterpret_cast<keymaster_block_mode_t*>(malloc(*modes_length * sizeof(**modes)));
    if (!*modes)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *modes);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::get_supported_padding_modes(const keymaster1_device_t* dev,
                                                                   keymaster_algorithm_t algorithm,
                                                                   keymaster_purpose_t purpose,
                                                                   keymaster_padding_t** modes,
                                                                   size_t* modes_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!modes || !modes_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SupportedPaddingModesRequest request;
    request.algorithm = algorithm;
    request.purpose = purpose;
    SupportedPaddingModesResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK) {
        printf("get_supported_padding_modes failed with %d", response.error);
        return response.error;
    }

    *modes_length = response.results_length;
    *modes = reinterpret_cast<keymaster_padding_t*>(malloc(*modes_length * sizeof(**modes)));
    if (!*modes)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *modes);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::get_supported_digests(const keymaster1_device_t* dev,
                                                             keymaster_algorithm_t algorithm,
                                                             keymaster_purpose_t purpose,
                                                             keymaster_digest_t** digests,
                                                             size_t* digests_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!digests || !digests_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SupportedDigestsRequest request;
    request.algorithm = algorithm;
    request.purpose = purpose;
    SupportedDigestsResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK) {
        printf("get_supported_digests failed with %d", response.error);
        return response.error;
    }

    *digests_length = response.results_length;
    *digests = reinterpret_cast<keymaster_digest_t*>(malloc(*digests_length * sizeof(**digests)));
    if (!*digests)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *digests);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::get_supported_import_formats(
    const keymaster1_device_t* dev, keymaster_algorithm_t algorithm,
    keymaster_key_format_t** formats, size_t* formats_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!formats || !formats_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SupportedImportFormatsRequest request;
    request.algorithm = algorithm;
    SupportedImportFormatsResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK) {
        printf("get_supported_import_formats failed with %d", response.error);
        return response.error;
    }

    *formats_length = response.results_length;
    *formats =
        reinterpret_cast<keymaster_key_format_t*>(malloc(*formats_length * sizeof(**formats)));
    if (!*formats)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + response.results_length, *formats);
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::get_supported_export_formats(
    const keymaster1_device_t* dev, keymaster_algorithm_t algorithm,
    keymaster_key_format_t** formats, size_t* formats_length) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!formats || !formats_length)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    SupportedExportFormatsRequest request;
    request.algorithm = algorithm;
    SupportedExportFormatsResponse response;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK) {
        printf("get_supported_export_formats failed with %d", response.error);
        return response.error;
    }

    *formats_length = response.results_length;
    *formats =
        reinterpret_cast<keymaster_key_format_t*>(malloc(*formats_length * sizeof(**formats)));
    if (!*formats)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    std::copy(response.results, response.results + *formats_length, *formats);
    return KM_ERROR_OK;
}


/* static */
keymaster_error_t TrustyKeymasterDevice::add_rng_entropy(const keymaster1_device_t* dev,
                                                       const uint8_t* data, size_t data_length) {
    if (!dev ||!data)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;


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

static keymaster_key_characteristics_t* BuildCharacteristics(const AuthorizationSet& hw_enforced,
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

template <typename RequestType>
static void AddClientAndAppData(const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
                         RequestType* request) {
    request->additional_params.Clear();
    if (client_id)
        request->additional_params.push_back(TAG_APPLICATION_ID, *client_id);
    if (app_data)
        request->additional_params.push_back(TAG_APPLICATION_DATA, *app_data);
}

keymaster_error_t TrustyKeymasterDevice::configure(){
    ConfigureResponse response;
    ConfigureRequest request;
    request.os_patchlevel = 0;
    request.os_version= 0;

    keymaster_error_t err = Send(request, &response);
    if (err != KM_ERROR_OK) {
        ALOGE("Got error %d from send", err);
        return err;
    }

    if (response.error != KM_ERROR_OK)
        return response.error;

    return KM_ERROR_OK;
}


/* static */
keymaster_error_t TrustyKeymasterDevice::generate_key(
    const keymaster1_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t** characteristics) {
    if (!dev || !params)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!key_blob)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    GenerateKeyRequest request;
    request.key_description.Reinitialize(*params);

    GenerateKeyResponse response;

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
        *characteristics = BuildCharacteristics(response.enforced, response.unenforced);
        if (!*characteristics)
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::get_key_characteristics(
    const keymaster1_device_t* dev, const keymaster_key_blob_t* key_blob,
    const keymaster_blob_t* client_id, const keymaster_blob_t* app_data,
    keymaster_key_characteristics_t** characteristics) {
    if (!dev || !key_blob || !key_blob->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!characteristics)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

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

    *characteristics = BuildCharacteristics(response.enforced, response.unenforced);
    if (!*characteristics)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::import_key(
    const keymaster1_device_t* dev, const keymaster_key_param_set_t* params,
    keymaster_key_format_t key_format, const keymaster_blob_t* key_data,
    keymaster_key_blob_t* key_blob, keymaster_key_characteristics_t** characteristics) {
    if (!dev || !params || !key_data)
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

/* static */
keymaster_error_t TrustyKeymasterDevice::export_key(const keymaster1_device_t* dev,
                                                  keymaster_key_format_t export_format,
                                                  const keymaster_key_blob_t* key_to_export,
                                                  const keymaster_blob_t* client_id,
                                                  const keymaster_blob_t* app_data,
                                                  keymaster_blob_t* export_data) {
    if (!dev || !key_to_export || !key_to_export->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!export_data)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

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

/* static */
keymaster_error_t TrustyKeymasterDevice::delete_key(const keymaster1_device_t* dev,
                                                  const keymaster_key_blob_t* key) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    // we have nothing to delete
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::delete_all_keys(const keymaster1_device_t* dev) {
    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    // we have nothing to delete
    return KM_ERROR_OK;
}

/* static */
keymaster_error_t TrustyKeymasterDevice::begin(const keymaster1_device_t* dev,
                                             keymaster_purpose_t purpose,
                                             const keymaster_key_blob_t* key,
                                             const keymaster_key_param_set_t* in_params,
                                             keymaster_key_param_set_t* out_params,
                                             keymaster_operation_handle_t* operation_handle) {
    if (!dev || !key || !key->key_material)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!operation_handle)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

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
keymaster_error_t TrustyKeymasterDevice::update(const keymaster1_device_t* dev,
                                              keymaster_operation_handle_t operation_handle,
                                              const keymaster_key_param_set_t* in_params,
                                              const keymaster_blob_t* input, size_t* input_consumed,
                                              keymaster_key_param_set_t* out_params,
                                              keymaster_blob_t* output) {

    if (!dev || !input)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

    if (!input_consumed)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

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
keymaster_error_t TrustyKeymasterDevice::finish(const keymaster1_device_t* dev,
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

/* static */
keymaster_error_t TrustyKeymasterDevice::abort(const keymaster1_device_t* dev,
                                             keymaster_operation_handle_t operation_handle) {

    if (!dev)
        return KM_ERROR_UNEXPECTED_NULL_POINTER;

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

hw_device_t* TrustyKeymasterDevice::hw_device() {
    return &device_.common;
}

/* static */
int TrustyKeymasterDevice::close_device(hw_device_t* dev) {
    delete reinterpret_cast<TrustyKeymasterDevice*>(dev);
    return 0;
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
        ALOGE("Received %d byte response\n", rsp_size);
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
