#ifndef FUNCTION_DB_H
#define FUNCTION_DB_H

#include <map>
#include <list>
#include "common.h"

class FunctionDB{

      std::map<ProcedureSignature, std::list<ServerInfo>* > function_map;
    public:
      struct ServerInfo locate(struct ProcedureSignature);
      void register_function(struct ProcedureSignature, struct ServerInfo);
      void print();
};

#endif
