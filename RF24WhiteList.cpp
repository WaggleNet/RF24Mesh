#include "RF24WhiteList.h"

RF24WhiteList::RF24WhiteList()
{
    for (uint16_t i = 0; i < MAX_NODES_NUM; ++i) {_whitelist = 0;}
    _len = 0;
}


RF24WhiteList::RF24WhiteList(nodeid_t* whitelist, uint16_t len)
{
    memcpy(_whitelist, whitelist, MAX_NODES_NUM * sizeof(nodeid_t));
    _len = len;
}


bool RF24WhiteList::find(nodeid_t node_id)
{
    for (uint16_t i = 0; i < _len; ++i)
    {
        if (_whitelist[i] == node_id) {return true;}
    }
    return false;
}


bool RF24WhiteList::add(nodeid_t node_id)
{
    if (_len >= MAX_NODES_NUM) {return false;}
    _whitelist[_len] = node_id;
    return true;
}


bool RF24WhiteList::remove(nodeid_t node_id)
{
    for (uint16_t i = 0; i < _len; ++i)
    {
        if (_whitelist[i] == node_id)
        {
            --_len;
            _whitelist[i] = _whitelist[_len];
            return true;
        }
    }
    return false;
}


uint16_t RF24WhiteList::getLength()
{
    return _len;
}
