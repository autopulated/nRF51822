/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __NRF_SERVICE_DISCOVERY_H__
#define __NRF_SERVICE_DISCOVERY_H__

#include "ble.h"
#include "ServiceDiscovery.h"
#include "nRFDiscoveredCharacteristic.h"

void bleGattcEventHandler(const ble_evt_t *p_ble_evt);

class nRFServiceDiscovery : public ServiceDiscovery
{
public:
    static const uint16_t SRV_DISC_START_HANDLE             = 0x0001; /**< The start handle value used during service discovery. */
    static const uint16_t SRV_DISC_END_HANDLE               = 0xFFFF; /**< The end handle value used during service discovery. */

public:
    static const unsigned BLE_DB_DISCOVERY_MAX_SRV          = 4;      /**< Maximum number of services we can retain information for after a single discovery. */
    static const unsigned BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV = 4;      /**< Maximum number of characteristics per service we can retain information for. */

public:
    nRFServiceDiscovery() : serviceIndex(0),
                               numServices(0),
                               characteristicIndex(0),
                               numCharacteristics(0),
                               state(INACTIVE),
                               services(),
                               characteristics(),
                               serviceUUIDDiscoveryQueue(this),
                               charUUIDDiscoveryQueue(this),
                               onTerminationCallback(NULL) {
        /* empty */
    }

public:
    ble_error_t launchCharacteristicDiscovery(Gap::Handle_t connectionHandle, Gap::Handle_t startHandle, Gap::Handle_t endHandle);

public:
    void setupDiscoveredServices(const ble_gattc_evt_prim_srvc_disc_rsp_t *response);
    void setupDiscoveredCharacteristics(const ble_gattc_evt_char_disc_rsp_t *response);

    void triggerServiceUUIDDiscovery(void);
    void processDiscoverUUIDResponse(const ble_gattc_evt_char_val_by_uuid_read_rsp_t *response);
    void removeFirstServiceNeedingUUIDDiscovery(void);

    void terminateServiceDiscovery(void) {
        bool wasActive = isActive();
        state = INACTIVE;

        if (wasActive && onTerminationCallback) {
            onTerminationCallback(connHandle);
        }
    }

    void terminateCharacteristicDiscovery(void) {
        if (state == CHARACTERISTIC_DISCOVERY_ACTIVE) {
            state = SERVICE_DISCOVERY_ACTIVE;
        }
        serviceIndex++; /* Progress service index to keep discovery alive. */
    }

    bool isActive(void) const {
        return state != INACTIVE;
    }

    void setOnTermination(TerminationCallback_t callback) {
        onTerminationCallback = callback;
    }

private:
    void resetDiscoveredServices(void) {
        numServices  = 0;
        serviceIndex = 0;
    }

    void resetDiscoveredCharacteristics(void) {
        numCharacteristics  = 0;
        characteristicIndex = 0;
    }

public:
    void serviceDiscoveryStarted(Gap::Handle_t connectionHandle) {
        connHandle = connectionHandle;
        resetDiscoveredServices();
        state = SERVICE_DISCOVERY_ACTIVE;
    }

private:
    void characteristicDiscoveryStarted(Gap::Handle_t connectionHandle) {
        connHandle = connectionHandle;
        resetDiscoveredCharacteristics();
        state = CHARACTERISTIC_DISCOVERY_ACTIVE;
    }

private:
    /**
     * A datatype to contain service-indices for which long UUIDs need to be
     * discovered using read_val_by_uuid().
     */
    class ServiceUUIDDiscoveryQueue {
    public:
        ServiceUUIDDiscoveryQueue(nRFServiceDiscovery *parent) :
            numIndices(0),
            serviceIndices(),
            parentDiscoveryObject(parent) {
            /* empty */
        }

    public:
        void reset(void) {
            numIndices = 0;
            for (unsigned i = 0; i < BLE_DB_DISCOVERY_MAX_SRV; i++) {
                serviceIndices[i] = INVALID_INDEX;
            }
        }
        void enqueue(int serviceIndex) {
            serviceIndices[numIndices++] = serviceIndex;
        }
        int dequeue(void) {
            if (numIndices == 0) {
                return INVALID_INDEX;
            }

            unsigned valueToReturn = serviceIndices[0];
            numIndices--;
            for (unsigned i = 0; i < numIndices; i++) {
                serviceIndices[i] = serviceIndices[i + 1];
            }

            return valueToReturn;
        }
        unsigned getFirst(void) const {
            return serviceIndices[0];
        }
        size_t getCount(void) const {
            return numIndices;
        }

        /**
         * Trigger UUID discovery for the first of the enqueued ServiceIndices.
         */
        void triggerFirst(void);

    private:
        static const int INVALID_INDEX = -1;

    private:
        size_t numIndices;
        int    serviceIndices[BLE_DB_DISCOVERY_MAX_SRV];

        nRFServiceDiscovery *parentDiscoveryObject;
    };
    friend class ServiceUUIDDiscoveryQueue;

    /**
     * A datatype to contain characteristic-indices for which long UUIDs need to
     * be discovered using read_val_by_uuid().
     */
    class CharUUIDDiscoveryQueue {
    public:
        CharUUIDDiscoveryQueue(nRFServiceDiscovery *parent) :
            numIndices(0),
            charIndices(),
            parentDiscoveryObject(parent) {
            /* empty */
        }

    public:
        void reset(void) {
            numIndices = 0;
            for (unsigned i = 0; i < BLE_DB_DISCOVERY_MAX_SRV; i++) {
                charIndices[i] = INVALID_INDEX;
            }
        }
        void enqueue(int serviceIndex) {
            charIndices[numIndices++] = serviceIndex;
        }
        int dequeue(void) {
            if (numIndices == 0) {
                return INVALID_INDEX;
            }

            unsigned valueToReturn = charIndices[0];
            numIndices--;
            for (unsigned i = 0; i < numIndices; i++) {
                charIndices[i] = charIndices[i + 1];
            }

            return valueToReturn;
        }
        unsigned getFirst(void) const {
            return charIndices[0];
        }
        size_t getCount(void) const {
            return numIndices;
        }

        /**
         * Trigger UUID discovery for the first of the enqueued charIndices.
         */
        void triggerFirst(void);

    private:
        static const int INVALID_INDEX = -1;

    private:
        size_t numIndices;
        int    charIndices[BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV];

        nRFServiceDiscovery *parentDiscoveryObject;
    };
    friend class CharUUIDDiscoveryQueue;

private:
    friend void bleGattcEventHandler(const ble_evt_t *p_ble_evt);
    void progressCharacteristicDiscovery(void);
    void progressServiceDiscovery(void);

private:
    uint8_t  serviceIndex;        /**< Index of the current service being discovered. This is intended for internal use during service discovery.*/
    uint8_t  numServices;         /**< Number of services at the peers GATT database.*/
    uint8_t  characteristicIndex; /**< Index of the current characteristic being discovered. This is intended for internal use during service discovery.*/
    uint8_t  numCharacteristics;  /**< Number of characteristics within the service.*/

    enum State_t {
        INACTIVE,
        SERVICE_DISCOVERY_ACTIVE,
        CHARACTERISTIC_DISCOVERY_ACTIVE,
        DISCOVER_SERVICE_UUIDS,
        DISCOVER_CHARACTERISTIC_UUIDS,
    } state;

    DiscoveredService           services[BLE_DB_DISCOVERY_MAX_SRV];  /**< Information related to the current service being discovered.
                                                                      *  This is intended for internal use during service discovery. */
    nRFDiscoveredCharacteristic characteristics[BLE_DB_DISCOVERY_MAX_CHAR_PER_SRV];

    ServiceUUIDDiscoveryQueue   serviceUUIDDiscoveryQueue;
    CharUUIDDiscoveryQueue      charUUIDDiscoveryQueue;

    TerminationCallback_t       onTerminationCallback;
};

#endif /*__NRF_SERVICE_DISCOVERY_H__*/