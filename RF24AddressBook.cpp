#include "RF24AddressBook.h"

void AddressBook::begin() {
    list = (MeshAddress *) malloc(sizeof(MeshAddress));
    top = 0;
}

int AddressBook::alloc(nodeid_t nodeID, address_t address) {
    for (uint8_t i=0; i < top; i++)
        if (list[i].nodeID == nodeID) {
            list[i].address = address;
            list[i].lastRenew = millis();
            return 1; // For reusing current address
        }
    // Now handle getting a new address
    list[top].address = address;
    list[top].nodeID = nodeID;
    list[top].lastRenew = millis();
    top ++;
    list = (MeshAddress*)realloc(list, (top+1)*sizeof(MeshAddress));
    return 1;
}

address_t AddressBook::lookup_addr(nodeid_t nodeID) {
    for (uint8_t i=0; i < top; i++)
        if (list[i].nodeID == nodeID)
            return list[i].address;
    return 0;
}

nodeid_t AddressBook::lookup_id(address_t address) {
    for (uint8_t i=0; i < top; i++)
        if (list[i].address == address)
            return list[i].nodeID;
    return 0;
}

int AddressBook::renew(nodeid_t nodeID) {
    for (uint8_t i=0; i < top; i++)
        if (list[i].nodeID == nodeID) {
            list[i].lastRenew = millis();
            return 0;
        }
    return -1;
}

int AddressBook::release(address_t address) {
    for (uint8_t i=0; i<top; i++)
        if (list[i].address == address) {
            // Now free the memory from the stack
            for (uint8_t j=i+1; j<top; j++)
                list[j-1] = list[j];
            top --;
            list = (MeshAddress*) realloc(list, (top+1)*sizeof(MeshAddress));
            return 0;
        }
    return -1;
}

// WARNING: Time consuming operation. Only call once in a while.
int AddressBook::prune() {
    int counter = 0;
    uint32_t time_ms = millis();
    // Pass 1: Count number of nodes STAYING in the list
    // It should not have expired and should not have registered AFTER current time
    for (uint8_t i = 0; i < top; i++) {
        if ((long)(time_ms - list[i].lastRenew) < MESH_ADDRESS_EXPIRY && list[i].lastRenew < time_ms) {
            counter ++;
        } else {
            #if defined (MESH_DEBUG_PRINTF)
            printf("[DHCP] Pruning address 0x%x\n", list[i].nodeID);
            #elif defined (MESH_DEBUG_SERIAL)
                Serial.print(F("[DHCP] Pruning address 0x"));
                    Serial.println(list[i].nodeID, HEX);
            #endif
        }
    }
    auto newlist = (MeshAddress*) malloc((counter + 1) * sizeof(MeshAddress));
    if (top == counter) return counter;
    top = counter;
    // Serial.print(F("Pruning addressbook to "));
    // Serial.println(top);
    counter = 0;
    for (uint8_t i = 0; i < top; i++) {
        if ((long)(time_ms - list[i].lastRenew) < MESH_ADDRESS_EXPIRY && list[i].lastRenew < time_ms) {
            newlist[counter] = list[i];
            counter ++;
        }
    }
    free(list);
    list = newlist;
    return counter;
}

void AddressBook::loadFromFile() {
    #if defined (__linux) && !defined(__ARDUINO_X86__)
    	std::ifstream infile ("dhcplist.txt",std::ifstream::binary);
    	if(!infile){ return; }

        list[top].nodeID = 255;
    	list[top].address = 01114;

    	infile.seekg(0,infile.end);
    	int length = infile.tellg();
    	infile.seekg(0,infile.beg);

    	list = (MeshAddress*) realloc(list,length + sizeof(MeshAddress));

    	top = length/sizeof(MeshAddress);
    	for(int i=0; i<top; i++) {
    		infile.read( (char*)&list[i],sizeof(MeshAddress));
    	}
    	infile.close();
    #endif
}

void AddressBook::saveToFile() {
    #if defined (__linux)  && !defined(__ARDUINO_X86__)
    	std::ofstream outfile ("dhcplist.txt",std::ofstream::binary | std::ofstream::trunc);

    	//printf("writingToFile %d  0%o size %d\n",list[0].nodeID,list[0].address,sizeof(MeshAddress));

    	for(int i=0; i< top; i++){
    		outfile.write( (char*)&list[i],sizeof(MeshAddress));
        }
    	outfile.close();

    	/*MeshAddress aList;
    	std::ifstream infile ("dhcplist.txt",std::ifstream::binary);
    	infile.seekg(0,infile.end);
    	int length = infile.tellg();
    	infile.seekg(0,infile.beg);
    	//list = (MeshAddress*)malloc(length);

    	//infile.read( (char*)&list,length);
    	infile.read( (char*)&aList,sizeof(MeshAddress));
    	 //top = length/sizeof(MeshAddress);
    	//for(int i=0; i< top; i++){
    	printf("ID: %d  ADDR: 0%o  \n",aList.nodeID,aList.address);
    	//}
    	infile.close();*/
    #endif
}

int AddressBook::count() {
    return top;
}

MeshAddress& AddressBook::operator[](int index) {
    if (index >= top) {
        // Handle overflow case
        return list[0];
    }
    return list[index];
}
