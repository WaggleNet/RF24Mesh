#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "RF24Mesh_config.h"

#define MAX_NODES_NUM           256


class RF24WhiteList
{
    public:
        // default constructor, initialize all entries to 0
        RF24WhiteList();
        // initialize the whitelist from a given array
        RF24WhiteList(nodeid_t* whitelist, uint16_t len);
        bool find(nodeid_t node_id);
        bool add(nodeid_t node_id);
        bool remove(nodeid_t node_id);
        uint16_t getLength();

    private:
        nodeid_t _whitelist[MAX_NODES_NUM];
        uint16_t _len;
}

