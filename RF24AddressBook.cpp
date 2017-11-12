#include "RF24AddressBook.h"

void AddressBook::begin() {
    list = (MeshAddress *) malloc(2*sizeof(MeshAddress));
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
}

address_t AddressBook::lookup_addr(nodeid_t nodeID) {
    for (uint8_t i=0; i < top; i++)
        if (list[i].nodeID == nodeID)
            return list[i].address;
    return -1;
}

nodeid_t AddressBook::lookup_id(address_t address) {
    for (uint8_t i=0; i < top; i++)
        if (list[i].address == address)
            return list[i].nodeID;
    return -1;
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
    uint8_t counter = 0;
    for (uint8_t i=0; i<top; i++) {
        if ((long)(millis()-list[i].lastRenew) >= MESH_ADDRESS_EXPIRY) {
            release(list[i].address);
            i--; // Because it's now deleted, i represents new member
        }
    }
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
    	for(int i=0; i<top; i++){
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
