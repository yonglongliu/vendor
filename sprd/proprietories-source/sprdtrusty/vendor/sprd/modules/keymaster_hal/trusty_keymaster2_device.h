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

#ifndef EXTERNAL_KEYMASTER_TRUSTY_KEYMASTER2_DEVICE_H_
#define EXTERNAL_KEYMASTER_TRUSTY_KEYMASTER2_DEVICE_H_

#include <hardware/keymaster2.h>

#include "trusty_keymaster_messages.h"

#include "keymaster_ipc.h"

namespace keymaster {

/**
 * Software OpenSSL-based Keymaster device.
 *
 * IMPORTANT MAINTAINER NOTE: Pointers to instances of this class must be castable to hw_device_t
 * and keymaster_device. This means it must remain a standard layout class (no virtual functions and
 * no data members which aren't standard layout), and device_ must be the first data member.
 * Assertions in the constructor validate compliance with those constraints.
 */
class TrustyKeymasterDevice {
  public:
    /*
     * These are the only symbols that will be exported by libtrustykeymaster.  All functionality
     * can be reached via the function pointers in device_.
     */
    __attribute__((visibility("default"))) explicit TrustyKeymasterDevice(const hw_module_t* module);
    __attribute__((visibility("default"))) hw_device_t* hw_device();

    ~TrustyKeymasterDevice();
    keymaster2_device_t* device() {
        return &device_;
    }

    keymaster_error_t session_error() { return error_; }

  private:
    static keymaster_error_t Send(uint32_t command, const Serializable& request,
                           KeymasterResponse* response);
    static keymaster_error_t Send(const GenerateKeyRequest& request, GenerateKeyResponse* response) {
        return Send(KM_GENERATE_KEY, request, response);
    }
    static keymaster_error_t Send(const BeginOperationRequest& request, BeginOperationResponse* response) {
        return Send(KM_BEGIN_OPERATION, request, response);
    }
    static keymaster_error_t Send(const UpdateOperationRequest& request,
                           UpdateOperationResponse* response) {
        return Send(KM_UPDATE_OPERATION, request, response);
    }
    static keymaster_error_t Send(const FinishOperationRequest& request,
                           FinishOperationResponse* response) {
        return Send(KM_FINISH_OPERATION, request, response);
    }
    static keymaster_error_t Send(const ImportKeyRequest& request, ImportKeyResponse* response) {
        return Send(KM_IMPORT_KEY, request, response);
    }
    static keymaster_error_t Send(const ExportKeyRequest& request, ExportKeyResponse* response) {
        return Send(KM_EXPORT_KEY, request, response);
    }
    static keymaster_error_t Send(const GetVersionRequest& request, GetVersionResponse* response) {
        return Send(KM_GET_VERSION, request, response);
    }
    static keymaster_error_t Send(const AbortOperationRequest& request,
                          AbortOperationResponse* response) {
        return Send(KM_ABORT_OPERATION, request, response);
    }
    static keymaster_error_t Send(const AddEntropyRequest& request,
                          AddEntropyResponse* response) {
        return Send(KM_ADD_RNG_ENTROPY, request, response);
    }
    static keymaster_error_t Send(const GetKeyCharacteristicsRequest& request,
                          GetKeyCharacteristicsResponse* response) {
        return Send(KM_GET_KEY_CHARACTERISTICS, request, response);
    }
    static keymaster_error_t Send(const AttestKeyRequest& request,
                          AttestKeyResponse* response) {
        return Send(KM_ATTEST_KEY, request, response);
    }
    static keymaster_error_t Send(const UpgradeKeyRequest& request,
                          UpgradeKeyResponse* response) {
        return Send(KM_UPGRADE_KEY, request, response);
    }
    static keymaster_error_t Send(const ConfigureRequest& request,
                          ConfigureResponse* response) {
        return Send(KM_CONFIGURE, request, response);
    }
    /*
     * These static methods are the functions referenced through the function pointers in
     * keymaster_device.  They're all trivial wrappers.
     */
    static int close_device(hw_device_t* dev);
    static keymaster_error_t configure(const keymaster2_device_t* dev,
                                   const keymaster_key_param_set_t* params);
    static keymaster_error_t add_rng_entropy(const keymaster2_device_t* dev, const uint8_t* data,
                                             size_t data_length);
    static keymaster_error_t generate_key(const keymaster2_device_t* dev,
                                          const keymaster_key_param_set_t* params,
                                          keymaster_key_blob_t* key_blob,
                                          keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t get_key_characteristics(const keymaster2_device_t* dev,
                                                     const keymaster_key_blob_t* key_blob,
                                                     const keymaster_blob_t* client_id,
                                                     const keymaster_blob_t* app_data,
                                                     keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t import_key(const keymaster2_device_t* dev,  //
                                        const keymaster_key_param_set_t* params,
                                        keymaster_key_format_t key_format,
                                        const keymaster_blob_t* key_data,
                                        keymaster_key_blob_t* key_blob,
                                        keymaster_key_characteristics_t* characteristics);
    static keymaster_error_t import_key(const keymaster_key_param_set_t* params,
                                        keymaster_key_format_t key_format,
                                        const keymaster_blob_t* key_data,
                                        keymaster_key_blob_t* key_blob,
                                        keymaster_key_characteristics_t** characteristics);
    static keymaster_error_t export_key(const keymaster2_device_t* dev,  //
                                        keymaster_key_format_t export_format,
                                        const keymaster_key_blob_t* key_to_export,
                                        const keymaster_blob_t* client_id,
                                        const keymaster_blob_t* app_data,
                                        keymaster_blob_t* export_data);
    static keymaster_error_t attest_key(const keymaster2_device_t* dev,
                                    const keymaster_key_blob_t* key_to_attest,
                                    const keymaster_key_param_set_t* attest_params,
                                    keymaster_cert_chain_t* cert_chain);
    static keymaster_error_t upgrade_key(const keymaster2_device_t* dev,
                                     const keymaster_key_blob_t* key_to_upgrade,
                                     const keymaster_key_param_set_t* upgrade_params,
                                     keymaster_key_blob_t* upgraded_key);
    static keymaster_error_t delete_key(const keymaster2_device_t* dev,
                                    const keymaster_key_blob_t* key);
    static keymaster_error_t delete_all_keys(const keymaster2_device_t* dev);
    static keymaster_error_t begin(const keymaster2_device_t* dev, keymaster_purpose_t purpose,
                                   const keymaster_key_blob_t* key,
                                   const keymaster_key_param_set_t* in_params,
                                   keymaster_key_param_set_t* out_params,
                                   keymaster_operation_handle_t* operation_handle);
    static keymaster_error_t update(const keymaster2_device_t* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* input, size_t* input_consumed,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output);
    static keymaster_error_t finish(const keymaster2_device_t* dev,
                                keymaster_operation_handle_t operation_handle,
                                const keymaster_key_param_set_t* in_params,
                                const keymaster_blob_t* input, const keymaster_blob_t* signature,
                                keymaster_key_param_set_t* out_params, keymaster_blob_t* output);
    static keymaster_error_t finish(const keymaster2_device_t* dev,  //
                                    keymaster_operation_handle_t operation_handle,
                                    const keymaster_key_param_set_t* in_params,
                                    const keymaster_blob_t* signature,
                                    keymaster_key_param_set_t* out_params,
                                    keymaster_blob_t* output);
    static keymaster_error_t abort(const keymaster2_device_t* dev,
                                   keymaster_operation_handle_t operation_handle);

    keymaster2_device_t device_;
    keymaster_error_t error_;
    int32_t message_version_;
    static bool configured_;
};

}  // namespace keymaster

#endif  // EXTERNAL_KEYMASTER_TRUSTY_KEYMASTER_DEVICE_H_
