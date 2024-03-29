#include "function_db.h"
#include <iostream>
#include <algorithm>

using namespace std;

int FunctionDB::register_function(struct ProcedureSignature signatrue, struct ServerInfo info){
    if (function_map.count(signatrue) == 0) {
        // key not existed
        list<ServerInfo> *info_list = new list<ServerInfo>();
        info_list->push_back(info);

        function_map[signatrue] = info_list;
    } else {
        list<ServerInfo> *info_list = function_map[signatrue];

        for (std::list<ServerInfo>::iterator it= info_list->begin(); it != info_list->end(); ++it){
            if (it->host == info.host && it->port == info.port){
                return REGISTER_DUPLICATE;
            }
        }

        info_list->push_back(info);
    }
    return 0;
}

struct ServerInfo FunctionDB::locate(struct ProcedureSignature signatrue){
    ServerInfo info;
    info.host = "";

    if (function_map.count(signatrue) == 0) {
        return info;
    } else {
        list<ServerInfo> *info_list = function_map[signatrue];

        if (info_list->size() != 0){
            info = info_list->front();

            // round robin, pop the first and push it back to the end of the queue
            info_list->pop_front();
            info_list->push_back(info);
        }

        return info;
    }
}

std::list<ServerInfo>* FunctionDB::getList(struct ProcedureSignature signatrue) {
    if (function_map.count(signatrue) == 0) {
        return NULL;
    } else {
        return function_map[signatrue];
    }
}

void FunctionDB::print(){
    for (map<struct ProcedureSignature, std::list<ServerInfo>* >::iterator it=function_map.begin(); it!=function_map.end(); ++it) {
        cout << it->first.name << ", ";
        for (int *i = it->first.argTypes; *i != 0; i++) {
            cout << (unsigned int)*i << " ";
        } cout << " => ";
        cout << "list size " << it->second->size();
        for (std::list<ServerInfo>::iterator it1= it->second->begin(); it1 != it->second->end(); ++it1){
            cout << it1->host << " " <<  it1->port << " ";
        }
        cout << endl;
    }
}

void FunctionDB::remove(struct ProcedureSignature signatrue, struct ServerInfo info){
    if (function_map.count(signatrue) != 0) {
        list<ServerInfo> *info_list = function_map[signatrue];

        info_list->remove(info);
    }
}
