#ifndef FUNCTION_DB_H
#define FUNCTION_DB_H

#include <map>
#include <list>
#include "common.h"

class FunctionDB{

      std::map<ProcedureSignature, std::list<ServerInfo>* > function_map;
    public:
      struct ServerInfo locate(struct ProcedureSignature);
      int register_function(struct ProcedureSignature, struct ServerInfo);
      void remove(struct ProcedureSignature, struct ServerInfo);
      std::list<ServerInfo>* getList(struct ProcedureSignature);
      void print();
};

#endif
