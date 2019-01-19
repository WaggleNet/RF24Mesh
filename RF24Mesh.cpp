#include "RF24Mesh.h"
#include "RF24Mesh_config.h"
#if defined (__linux) && !defined(__ARDUINO_X86__)
#include <fstream>
#endif

RF24Mesh::RF24Mesh( RF24& _radio,RF24Network& _network ): radio(_radio),network(_network) {}

/*****************************************************/

bool RF24Mesh::begin(uint8_t channel, rf24_datarate_e data_rate, uint32_t timeout) {
    // delay(1); // Found problems w/SPIDEV & ncurses. Without this, getch() returns a stream of garbage
    bool result = 1;
    result &= radio.begin();
    radio_channel = channel;
    radio.setChannel(radio_channel);
    radio.setDataRate(data_rate);
    network.returnSysMsgs = 1;

    if (getNodeID()) { // Not master node
        mesh_address = MESH_DEFAULT_ADDRESS;
        if (!renewAddress(timeout))
            return 0;
    } else {
        #if !defined (RF24_TINY) && !defined(MESH_NOMASTER)
            addrBook.begin();
            loadDHCP();
        #endif
        mesh_address = 0;
        network.begin(mesh_address);
    }
    return result;
}

/*****************************************************/

uint8_t RF24Mesh::update() {
    uint8_t type = network.update();
    if (mesh_address == MESH_DEFAULT_ADDRESS) return type;

    #if !defined (RF24_TINY) && !defined(MESH_NOMASTER)
        if (type == NETWORK_REQ_ADDRESS) doDHCP = 1;

        if (!getNodeID()) {
            if (type == MESH_ADDR_LOOKUP || type == MESH_ID_LOOKUP) {
                // Lookup request
                RF24NetworkHeader header;
                memcpy(&header, &network.frame_buffer, sizeof(RF24NetworkHeader));
                header.to_node = header.from_node;

                uint32_t returnAddr;

                if (type==MESH_ADDR_LOOKUP) {
                    nodeid_t *node_id = (nodeid_t*)&network.frame_buffer[sizeof(RF24NetworkHeader)];
                    returnAddr = getAddress(*node_id);
                } else {
                    address_t *address = (address_t*)&network.frame_buffer[sizeof(RF24NetworkHeader)];
                    returnAddr = getNodeID(*address);
                }
                network.write(header,&returnAddr,sizeof(returnAddr));
            } else if (type == MESH_ADDR_RELEASE) {
                // Release Request
                uint16_t *fromAddr = (uint16_t*) network.frame_buffer;
                addrBook.release(*fromAddr);
            }
            #if !defined (ARDUINO_ARCH_AVR)
                else if (type == MESH_ADDR_CONFIRM) {
                    RF24NetworkHeader& header = *(RF24NetworkHeader*)network.frame_buffer;
                    if (header.from_node == lastAddress) {
                        setAddress(lastID,lastAddress);
                    }
                }
            #endif
        }
    #endif
    return type;
}

bool RF24Mesh::write(uint16_t to_node, const void* data, uint8_t msg_type, size_t size) {
    if (mesh_address == MESH_DEFAULT_ADDRESS) return 0;
    RF24NetworkHeader header(to_node,msg_type);
    return network.write(header,data,size);
}

/*****************************************************/

bool RF24Mesh::write(const void* data, uint8_t msg_type, size_t size, nodeid_t nodeID) {
    if (mesh_address == MESH_DEFAULT_ADDRESS) return 0;

    address_t toNode = 0;
    long lookupTimeout = millis()+ MESH_LOOKUP_TIMEOUT;
    unsigned long retryDelay = 50;

    if (nodeID)
        while ((toNode=getAddress(nodeID)) < 0) {
            if ((long)millis() > lookupTimeout || toNode == 0)
                return 0;
            retryDelay+=50;
            delay(retryDelay);
        }
    return write(toNode,data,msg_type,size);
}

/*****************************************************/

void RF24Mesh::setChannel(uint8_t _channel) {
    radio_channel = _channel;
    radio.setChannel(radio_channel);
    radio.startListening();
}

/*****************************************************/

void RF24Mesh::setChild(bool allow) {
    // Prevent old versions of RF24Network from throwing an error
    // Note to remove this ""if defined"" after a few releases from 1.0.1
    #if defined FLAG_NO_POLL
    network.networkFlags = allow ? network.networkFlags & ~FLAG_NO_POLL : network.networkFlags | FLAG_NO_POLL;
    #endif
}

/*****************************************************/

bool RF24Mesh::checkConnection() {
    uint8_t count = 3;
    bool ok = 0;
    while (count-- && mesh_address != MESH_DEFAULT_ADDRESS) {
        update();
        if (radio.rxFifoFull() || (network.networkFlags & 1))
            return 1;
        RF24NetworkHeader header(00, NETWORK_PING);
        ok = network.write(header,0,0);
        if (ok) break;
        delay(103);
    }
    if (!ok) radio.stopListening();
    return ok;
}

/*****************************************************/

address_t RF24Mesh::getAddress(nodeid_t nodeID) {
    #if !defined RF24_TINY && !defined(MESH_NOMASTER)
        if (!getNodeID()) { // Master Node
            return addrBook.lookup_addr(nodeID);
        }
    #endif
    if (mesh_address == MESH_DEFAULT_ADDRESS) return 0;
    if (!nodeID) return 0;
    RF24NetworkHeader header(00, MESH_ADDR_LOOKUP);
    if (network.write(header, &nodeID, sizeof(nodeID)+1)) {
        uint32_t timer = millis(), timeout = 150;
        while (network.update() != MESH_ADDR_LOOKUP)
            if (millis() - timer > timeout) return 0;
    } else return 0;
    uint32_t address = 0;
    memcpy(&address, network.frame_buffer + sizeof(RF24NetworkHeader),  sizeof(address));
    return address_t(address);
}

nodeid_t RF24Mesh::getNodeID(address_t address) {
    if (address == MESH_BLANK_ID) return _nodeID;
    if (address == 0) return 0;

    #if !defined RF24_TINY && !defined(MESH_NOMASTER)
        if (!mesh_address) { // Master Node
            return addrBook.lookup_id(address);
        }
    #endif

    if (mesh_address == MESH_DEFAULT_ADDRESS) return -1;
    RF24NetworkHeader header(00, MESH_ID_LOOKUP);
    if (network.write(header, &address, sizeof(address))) {
        uint32_t timer = millis(), timeout = 500;
        while (network.update() != MESH_ID_LOOKUP)
            if (millis()-timer > timeout) return -1;
        nodeid_t ID;
        memcpy(&ID, &network.frame_buffer[sizeof(RF24NetworkHeader)], sizeof(ID));
        return ID;
    }
    return 0;
}
/*****************************************************/

bool RF24Mesh::releaseAddress() {
    if (mesh_address == MESH_DEFAULT_ADDRESS) return 0;

    RF24NetworkHeader header(00, MESH_ADDR_RELEASE);
    if (network.write(header, 0, 0)) {
        network.begin(MESH_DEFAULT_ADDRESS);
        mesh_address = MESH_DEFAULT_ADDRESS;
        return 1;
    }
    return 0;
}

/*****************************************************/

uint16_t RF24Mesh::renewAddress(uint32_t timeout) {
    if (radio.available()) return 0;
    uint8_t reqCounter = 0;
    uint8_t totalReqs = 0;
    radio.stopListening();

    network.networkFlags |= 2;
    delay(10);

    network.begin(MESH_DEFAULT_ADDRESS);
    mesh_address = MESH_DEFAULT_ADDRESS;

    uint32_t start = millis();
    while (!requestAddress(reqCounter)) {
        if (millis()-start > timeout) return 0;
        delay(50 + ( (totalReqs+1)*(reqCounter+1)) * 2);
        (++reqCounter) = reqCounter%4;
        (++totalReqs) = totalReqs%10;
    }
    network.networkFlags &= ~2;
    return mesh_address;
}

/*****************************************************/

bool RF24Mesh::requestAddress(uint8_t level) {
    RF24NetworkHeader header(0100, NETWORK_POLL);
    // Find another radio, starting with level 0 multicast
    #if defined (MESH_DEBUG_SERIAL)
        Serial.print(millis());
        Serial.println(F(" MSH: Poll "));
    #endif
    network.multicast(header, 0, 0, level);

    uint32_t timr = millis();
    #define MESH_MAXPOLLS 4
    address_t contactNode[MESH_MAXPOLLS];
    uint8_t pollCount=0;

    while (1) {
        #if defined (MESH_DEBUG_SERIAL) || defined (MESH_DEBUG_PRINTF)
            bool goodSignal = radio.testRPD();
        #endif
        if (network.update() == NETWORK_POLL) {
            // Note down all nodes that responded me
            memcpy(&contactNode[pollCount], &network.frame_buffer[0], sizeof(uint16_t));
            ++pollCount;
            // Some Debug things
            #ifdef MESH_DEBUG_SERIAL
                Serial.print(millis());
                if (goodSignal) Serial.println(F(" MSH: Poll > -64dbm "));
                else Serial.println(F(" MSH: Poll < -64dbm "));
            #elif defined (MESH_DEBUG_PRINTF)
                if (goodSignal) printf("%u MSH: Poll > -64dbm\n", millis());
                else printf("%u MSH: Poll < -64dbm\n", millis());
            #endif
        }

        if (millis() - timr > 55 || pollCount >= MESH_MAXPOLLS ) {
            if (!pollCount) {
                #if defined (MESH_DEBUG_SERIAL)
                    Serial.print(millis());
                    Serial.print(F(" MSH: No poll from level "));
                    Serial.println(level);
                #elif defined (MESH_DEBUG_PRINTF)
                    printf("%u MSH: No poll from level %d\n", millis(), level);
                #endif
                return 0;
            } else {
                #if defined (MESH_DEBUG_SERIAL)
                    Serial.print(millis());
                    Serial.println(F(" MSH: Poll OK "));
                #elif defined (MESH_DEBUG_PRINTF)
                    printf("%u MSH: Poll OK\n", millis());
                #endif
                break;
            }
        }
    }

    #ifdef MESH_DEBUG_SERIAL
        Serial.print(millis());
        Serial.print(F(" MSH: Got poll from level ")); Serial.print(level);
        Serial.print(F(" count "));
        Serial.println(pollCount);
    #elif defined MESH_DEBUG_PRINTF
        printf("%u MSH: Got poll from level %d count %d\n", millis(), level, pollCount);
    #endif

    uint8_t type = 0;
    for (uint8_t i = 0; i<pollCount; i++) {
        // Request an address via the contact node
        header.type = NETWORK_REQ_ADDRESS;
        header.to_node = contactNode[i];
        // Do a direct write (no ack) to the contact node. Include the nodeId and address.

        nodeid_t my_id = getNodeID();
        network.write(header, &my_id, sizeof(my_id), contactNode[i]);
        #ifdef MESH_DEBUG_SERIAL
            Serial.print(millis());
            Serial.print(F(" MSH: Req addr from ")); Serial.println(contactNode[i], OCT);
        #elif defined MESH_DEBUG_PRINTF
            printf("%u MSH: Request address from: 0%o\n", millis(), contactNode[i]);
        #endif

        timr = millis();

        while (millis() - timr < 225)
            if ((type = network.update()) == NETWORK_ADDR_RESPONSE) {
                i = pollCount;
                break;
            }
        delay(5);
    }
    if (type != NETWORK_ADDR_RESPONSE) {
        return 0;
    }

    uint8_t registerAddrCount = 0;

    addrListStruct addrResponse;
    memcpy(&addrResponse, network.frame_buffer + sizeof(RF24NetworkHeader), sizeof(addrResponse));
    auto &newAddress = addrResponse.address;
    #ifdef MESH_DEBUG_SERIAL
        Serial.println();
        Serial.print(millis());
        Serial.print(" MSH: Received Address Response, addr=");
        Serial.print(addrResponse.address, OCT);
        Serial.print(", nodeID=");
        Serial.println(addrResponse.nodeID);
    #endif

    if (!newAddress || addrResponse.nodeID != getNodeID()) {
        #ifdef MESH_DEBUG_SERIAL
            Serial.print(millis());
            Serial.print(F(" MSH: Attempt Failed "));
            Serial.print(addrResponse.nodeID);
            Serial.print(", My NodeID ");
            Serial.println(getNodeID());
        #elif defined MESH_DEBUG_PRINTF
            printf("%u Response discarded, wrong node 0%o from node 0%o sending node 0%o id %d\n",millis(),newAddress,header.from_node,MESH_DEFAULT_ADDRESS,addrResponse.nodeID);
        #endif
        return 0;
    }

    #ifdef MESH_DEBUG_SERIAL
        Serial.print(millis());
        Serial.print(F(" Set address: "));
        Serial.println(newAddress, OCT);
    #elif defined (MESH_DEBUG_PRINTF)
        printf("Set address 0%o rcvd 0%o\n", mesh_address, newAddress);
    #endif
    mesh_address = newAddress;

    radio.stopListening();
    delay(10);
    network.begin(mesh_address);
    header.to_node = 00;
    header.type = MESH_ADDR_CONFIRM;

    while (!network.write(header,0,0)) {
        if (registerAddrCount++ >= 6) {
            network.begin(MESH_DEFAULT_ADDRESS);
            mesh_address = MESH_DEFAULT_ADDRESS;
            return 0;
        }
        delay(3);
    }
    return 1;
}

/*****************************************************/
/*
bool RF24Mesh::waitForAvailable(uint32_t timeout) {

unsigned long timer = millis();
while (millis()-timer < timeout) {
network.update();
if (network.available()) { return 1; }
}
if (network.available()) { return 1; }
else{  return 0; }
}
*/
/*****************************************************/

void RF24Mesh::setNodeID(nodeid_t nodeID) {
    _nodeID = nodeID;
}

/*****************************************************/

void RF24Mesh::setStaticAddress(nodeid_t nodeID, address_t Address) {
    setAddress(nodeID, Address);
}

/*****************************************************/

void RF24Mesh::setAddress(nodeid_t nodeID, address_t Address) {
    addrBook.alloc(nodeID, Address);
    #if defined (__linux)  && !defined(__ARDUINO_X86__)
        saveDHCP();
    #endif
}

/*****************************************************/

void RF24Mesh::loadDHCP() {
    addrBook.loadFromFile();
}

/*****************************************************/

void RF24Mesh::saveDHCP() {
    addrBook.saveToFile();
}

/*****************************************************/

#if !defined (RF24_TINY) && !defined(MESH_NOMASTER)

void RF24Mesh::DHCP() {
    if (!doDHCP) return;
    // Unflag DHCP Necessity
    doDHCP = 0;

    RF24NetworkHeader header;
    memcpy(&header, network.frame_buffer, sizeof(header));

    // Get the unique id of the requester
    uint8_t* buffer_pos = &(network.frame_buffer[sizeof(header)]);
    // nodeid_t from_id = *(nodeid_t*)buffer_pos;
    nodeid_t from_id;
    memcpy(&from_id, buffer_pos, sizeof(from_id));

    #if defined (MESH_DEBUG_PRINTF)
        printf("[DHCP] Request from ID %d\n", from_id);
    #elif defined (MESH_DEBUG_SERIAL)
        Serial.print("[DHCP] Request from ID ");
        Serial.println(from_id);
    #endif

    if (!from_id) return;

    uint16_t fwd_by = 0;
    uint8_t shiftVal = 0;
    bool extraChild = 0;

    if (header.from_node != MESH_DEFAULT_ADDRESS) {
        fwd_by = header.from_node;
        // Find out how many digits are in the octal address => bits to shift when adding a child node 1-5 (B001 to B101) to any address
        while (fwd_by >> shiftVal) shiftVal += 3;
    } else {
        // If request is coming from level 1, add an extra child to the master
        dprint("[DHCP] Adding extra child to master\n");
        extraChild = 1;
    }

    for (int i = MESH_MAX_CHILDREN + extraChild; i > 0; i--) { // For each of the possible addresses (5 max)
        address_t newAddress = fwd_by | (i << shiftVal);
        #if defined (MESH_DEBUG_PRINTF)
            printf("[DHCP] Trying new address %o\n", newAddress);
        #elif defined (MESH_DEBUG_SERIAL)
            Serial.print("[DHCP] Trying new address ");
            Serial.println(newAddress, OCT);
        #endif
        if (!newAddress) continue;

        nodeid_t current_user = addrBook.lookup_id(newAddress);

        bool found = (newAddress == MESH_DEFAULT_ADDRESS) || (current_user != from_id && current_user > 0);

        if (!found) {
            header.type = NETWORK_ADDR_RESPONSE;
            header.to_node = header.from_node;
            addrListStruct addrResponse;
            addrResponse.address = newAddress;
            addrResponse.nodeID = from_id;
            // This is a routed request to 00
            #ifdef MESH_DEBUG_PRINTF
                printf("[DHCP] Awarding Address %o to #%d\n", newAddress, from_id);
            #elif defined MESH_DEBUG_SERIAL
                Serial.print("[DHCP] Awarding Address ");
                Serial.print(newAddress, OCT);
                Serial.print(" to #");
                Serial.println(from_id);
            #endif
            if (header.from_node != MESH_DEFAULT_ADDRESS) { // Is NOT node 01 to 05
                delay(2);
                if (network.write(header,&addrResponse,sizeof(addrResponse))) {
                    // addrMap[from_id] = newAddress; //????
                } else {
                    network.write(header,&addrResponse,sizeof(addrResponse));
                }
            } else {
                delay(2);
                network.write(header, &addrResponse, sizeof(addrResponse), header.to_node);
                // addrMap[from_id] = newAddress;
            }
            uint32_t timer=millis();
            lastAddress = newAddress;
            lastID = from_id;
            while (network.update() != MESH_ADDR_CONFIRM)
                if (millis() - timer > network.routeTimeout) {
                    dprint("[DHCP] Not allocated: Found but timeout\n");
                    return;
                }
            setAddress(from_id,newAddress);
            #if defined (MESH_DEBUG_PRINTF)
                printf("Sent to 0%o phys: 0%o new: 0%o id: %d\n", header.to_node,MESH_DEFAULT_ADDRESS,newAddress,from_id);
            #elif defined (MESH_DEBUG_SERIAL)
                Serial.print("Sent to ");
                Serial.print(header.to_node, OCT);
                Serial.print(" new: ");
                Serial.print(newAddress, OCT);
                Serial.print(" id: ");
                Serial.println(from_id);
            #endif
            break;
        } else dprint("[DHCP] Not allocated: Not found\n");
    }
}

#endif //!defined (RF24_TINY) && !defined(MESH_NOMASTER)

/*****************************************************/
