#pragma once
#include "Arduino.h"
#include "RF24Mesh.h"
#include "RF24Mesh_config.h"

typedef struct MeshAddress {
    nodeid_t nodeID;
    address_t address;
    uint32_t lastRenew;
}

class AddressBook {
public:
    // Initialize the addressbook
    void begin();
    // Give some nodeID a new address. Return: New / Old Device
    int alloc(nodeid_t nodeID, address_t address);
    // Perform a lookup
    address_t lookup_addr(nodeid_t nodeID);
    nodeid_t lookup_id(address_t address)
    // Renews a node, when it's communicated. Auto called when alloc.
    int renew(nodeid_t nodeID);
    // Releases an address
    int release(address_t address);
    // Prune expired nodes. Returns # of expiries
    int prune();
    void saveToFile();
    void loadFromFile();
private:
    void add(nodeid_t nodeID, address_t address);
    MeshAddress *list;
    uint16_t top;
}
