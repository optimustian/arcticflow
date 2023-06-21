#include "SimAST2IR.h"

#include <iostream>
#include <set>
#include <stdexcept>

namespace XPUSchedulerSimulator {

void SIMIRBuilder::AST2CPPIR(SIMTranslationUnit *unit) {
  ir += EmitMainFunc(unit);
}

std::string SIMIRBuilder::EmitIRHeader() {
  return R"(
        #include <sys/time.h>
        #include <unistd.h>

        #include <atomic>
        #include <cstdint>
        #include <iostream>
        #include <map>
        #include <mutex>
        #include <queue>
        #include <set>
        #include <thread>
        #include <unordered_map>
        #include <vector>
    )";
}

std::string SIMIRBuilder::EmitHardwareEnum(SIMTranslationUnit *unit) {
  std::string ret = R"(
        enum class Hardware
        {
    )";
  for (auto *hardwareExpr : unit->hardware->exprs) {
    ret += "\t\t";
    ret += hardwareExpr->hardwareName->name;
    ret += ",";
    ret += "\n";
  }
  ret += "\t};\n\n";
  return ret;
}

std::string SIMIRBuilder::EmitHardwareCntMap(SIMTranslationUnit *unit) {
  std::string ret = "\tstd::map<Hardware, int32_t> hardwareCnt{";
  for (auto *hardwareExpr : unit->hardware->exprs) {
    ret += "{Hardware::";
    ret += hardwareExpr->hardwareName->name;
    ret += ", ";
    ret += std::to_string(hardwareExpr->hardwareCnt);
    ret += "}, ";
  }
  ret += "};\n";
  return ret;
}

std::string SIMIRBuilder::EmitOperatorFuncBody(SIMTranslationUnit *unit) {
  std::string ret;
  ret += R"(
        #define FUNC_BODY(_NAME, _TIME)                                     \
        void _NAME() {                                                      \
          struct timeval begin, now;                                        \
          gettimeofday(&begin, NULL);                                       \
          while (true) {                                                    \
            gettimeofday(&now, NULL);                                       \
            if (now.tv_sec * 1000000ULL + now.tv_usec >                     \
                begin.tv_sec * 1000000ULL + begin.tv_usec + _TIME * 1000) { \
              return;                                                       \
            }                                                               \
            usleep(10);                                                     \
          }                                                                 \
        }

  )";
  for (auto *operatorExpr : unit->op->exprs) {
    ret += StringFormat("\tFUNC_BODY(%s, %d)\n",
                        operatorExpr->opName->name.c_str(), operatorExpr->time);
  }
  return ret;
}

std::map<std::string, int32_t> g_OperatorTime;

std::string SIMIRBuilder::EmitOpToTimeMap(SIMTranslationUnit *unit) {
  std::string ret =
      R"(
        std::map<void *, std::pair<Hardware, int32_t>> operatorToHardwareTime{
  )";
  for (auto *operatorExpr : unit->op->exprs) {
    ret += StringFormat("\t\t{(void *)%s, {Hardware::%s, %d}},\n",
                        operatorExpr->opName->name.c_str(),
                        operatorExpr->hardwareName->name.c_str(),
                        operatorExpr->time);
    g_OperatorTime[operatorExpr->opName->name] = operatorExpr->time;
  }
  ret += "\t};\n\n";
  return ret;
}
std::string SIMIRBuilder::EmitInstanceMap(SIMTranslationUnit *unit) {
  std::string ret = R"(
        std::map<uint64_t, bool> aliveInstanceId;
        std::map<uint64_t, std::vector<uint64_t>> instancePreId, instancePostId;
        std::map<uint64_t, void *> instanceToOperator;
        uint64_t topInstanceId;
        volatile uint64_t simuDone;
        volatile uint64_t schedulerDone;
  )";
  return ret;
}
std::string SIMIRBuilder::EmitRegisterInstanceFunc(SIMTranslationUnit *unit) {
  std::string ret = R"(
        uint64_t registerInstance(void *op, const std::vector<uint64_t> &_instancePreId)
        {
            while (true) {
              aliveInstanceMutex.lock();
              if (aliveInstanceId.size() > 1023) {
                aliveInstanceMutex.unlock();
                usleep(10);
                continue;
              }
              aliveInstanceId[++topInstanceId] = 0;
              instancePreId[topInstanceId] = _instancePreId;
              instanceToOperator[topInstanceId] = op;
              for (auto preId : _instancePreId)
              {
                  instancePostId[preId].emplace_back(topInstanceId);
              }
              aliveInstanceMutex.unlock();
              break;
            }
            return topInstanceId;
        }
  )";
  return ret;
}

std::map<std::string, std::vector<std::string>>
GetOperatorGraph(const std::string &flowBlockName, SIMFlowBlock *block);

// key: foreachName, value: cppir
std::map<std::string, std::string> g_ForeachExprIR;
std::map<std::string, std::string> g_ForeachExprIRHeader;
// key: foreachName, value: count
std::map<std::string, int32_t> g_ForeachCnt;

std::string GetFlowCallOrOperatorCall(const std::string &currentInstance) {
  if (g_OperatorTime.count(currentInstance)) {
    return StringFormat("\t    uint64_t id_%s = registerInstance((void *)%s, "
                        "preid_%s);\n",
                        currentInstance.c_str(), currentInstance.c_str(),
                        currentInstance.c_str());
  } else if (g_ForeachCnt.count(currentInstance)) {
    std::string ret;
    for (int loopI = 0; loopI < g_ForeachCnt.at(currentInstance); loopI++) {
      ret += StringFormat("\t    auto id_%s_loop%d = %s(preid_%s);\n",
                          currentInstance.c_str(), loopI,
                          currentInstance.c_str(), currentInstance.c_str());
    }
    return ret;
  } else {
    return StringFormat("\t    auto id_%s = %s(preid_%s);\n",
                        currentInstance.c_str(), currentInstance.c_str(),
                        currentInstance.c_str());
  }
}

void DFS_RegisterInstanceCall(
    std::map<std::string, std::vector<std::string>> &operatorGraph,
    std::set<std::string> &visited, const std::string &currentInstance,
    std::string &ir) {
  if (visited.count(currentInstance)) {
    return;
  }
  auto &vec = operatorGraph.at(currentInstance);

  // 处理开头的图op
  if (vec.size() == 0) {
    ir += StringFormat(
        "\t    const std::vector<uint64_t> &preid_%s = _instancePreId;\n",
        currentInstance.c_str());
    ir += GetFlowCallOrOperatorCall(currentInstance);
    visited.insert(currentInstance);
    return;
  }

  // 处理当前图op的前驱
  ir += StringFormat("\t    std::vector<uint64_t> preid_%s;\n",
                     currentInstance.c_str());
  for (auto &preInstance : operatorGraph.at(currentInstance)) {
    if (!visited.count(preInstance)) {
      DFS_RegisterInstanceCall(operatorGraph, visited, preInstance, ir);
    }

    if (g_OperatorTime.count(preInstance)) {
      ir += StringFormat("\t    preid_%s.emplace_back(id_%s);\n",
                         currentInstance.c_str(), preInstance.c_str());
    } else if (g_ForeachCnt.count(preInstance)) {
      for (int loopI = 0; loopI < g_ForeachCnt.at(preInstance); loopI++) {
        ir += StringFormat(
            "\t    preid_%s.insert(preid_%s.end(), id_%s_loop%d.begin(), "
            "id_%s_loop%d.end());\n",
            currentInstance.c_str(), currentInstance.c_str(),
            preInstance.c_str(), loopI, preInstance.c_str(), loopI);
      }
    } else {
      ir += StringFormat("\t    preid_%s.insert(preid_%s.end(), id_%s.begin(), "
                         "id_%s.end());\n",
                         currentInstance.c_str(), currentInstance.c_str(),
                         preInstance.c_str(), preInstance.c_str());
    }
  }

  // 处理当前图op
  if (g_OperatorTime.count(currentInstance)) {
    ir += StringFormat(
        "\t    uint64_t id_%s = registerInstance((void *)%s, preid_%s);\n",
        currentInstance.c_str(), currentInstance.c_str(),
        currentInstance.c_str());
  } else if (g_ForeachCnt.count(currentInstance)) {
    for (int loopI = 0; loopI < g_ForeachCnt.at(currentInstance); loopI++) {
      ir += StringFormat("\t    auto id_%s_loop%d = %s(preid_%s);\n",
                         currentInstance.c_str(), loopI,
                         currentInstance.c_str(), currentInstance.c_str());
    }
  } else {
    ir += StringFormat("\t    auto id_%s = %s(preid_%s);\n",
                       currentInstance.c_str(), currentInstance.c_str(),
                       currentInstance.c_str());
  }
  visited.insert(currentInstance);
}

std::string GenRegisterInstanceCall(
    std::map<std::string, std::vector<std::string>> &operatorGraph) {
  std::set<std::string> visited;
  std::string ir;
  for (auto &[opName, preOpVec] : operatorGraph) {
    if (!visited.count(opName)) {
      DFS_RegisterInstanceCall(operatorGraph, visited, opName, ir);
    }
  }

  ir += "\n\t    std::vector<uint64_t> ret;";
  ir += "\n";
  for (auto &[opName, preOpVec] : operatorGraph) {
    if (g_OperatorTime.count(opName)) {
      ir += StringFormat("\t    ret.emplace_back(id_%s);", opName.c_str());
    } else if (g_ForeachCnt.count(opName)) {
      for (int32_t loopI = 0; loopI < g_ForeachCnt.at(opName); loopI++) {
        ir += StringFormat("\t    ret.insert(ret.end(), id_%s_loop%d.begin(), "
                           "id_%s_loop%d.end());\n",
                           opName.c_str(), loopI, opName.c_str(), loopI);
      }
    } else {
      ir += StringFormat("\t    ret.insert(ret.end(), id_%s.begin(), "
                         "id_%s.end());\n",
                         opName.c_str(), opName.c_str());
    }
    ir += "\n";
  }
  return ir;
}

void GetFlowExprGraph(std::map<std::string, std::vector<std::string>> &subRet,
                      SIMFlowExpression *&preExpr, SIMFlowExpression *flowExpr,
                      const int32_t beginExprId, int32_t &depthCnt,
                      const std::string &flowBlockName) {
  auto OperatorExprToString = [beginExprId, &depthCnt,
                               &flowBlockName](SIMFlowExpression *expr) {
    std::string opName;
    if (SIMFlowUnaryExpression *unaryExpr =
            dynamic_cast<SIMFlowUnaryExpression *>(expr)) {
      opName = unaryExpr->opName->name;
    } else if (SIMForeachExpression *foreachExpr =
                   dynamic_cast<SIMForeachExpression *>(expr)) {
      opName = StringFormat("%s_%dFE%d", flowBlockName.c_str(), beginExprId,
                            depthCnt);
      if (g_ForeachExprIR.count(opName) == 0) {
        std::string foreachIR;
        foreachIR += StringFormat(
            "\n\tstd::vector<uint64_t> %s(const std::vector<uint64_t> "
            "&_instancePreId) {\n",
            opName.c_str());
        auto operatorGraph = GetOperatorGraph(
            opName, dynamic_cast<SIMFlowBlock *>(foreachExpr->loopBlock));
        for (auto &[opExpr, preOpExprVec] : operatorGraph) {
          for (auto preOpExpr : preOpExprVec) {
            foreachIR += "\t//    " + opExpr + " <- " + preOpExpr + "\n";
          }
        }
        foreachIR += GenRegisterInstanceCall(operatorGraph);
        foreachIR += "\t    return ret;\n";
        foreachIR += "\t}\n";
        g_ForeachExprIR[opName] = foreachIR;
        g_ForeachExprIRHeader[opName] = StringFormat(
            "\n\tstd::vector<uint64_t> %s(const std::vector<uint64_t> "
            "&_instancePreId);",
            opName.c_str());
        g_ForeachCnt[opName] = foreachExpr->loopCnt;
      }
    } else {
      throw std::logic_error("Unsupported FlowExpression!");
    }
    return opName;
  };

  if (SIMFlowBinaryExpression *binaryExpr =
          dynamic_cast<SIMFlowBinaryExpression *>(flowExpr)) {
    GetFlowExprGraph(subRet, preExpr, binaryExpr->leftExpr, beginExprId,
                     depthCnt, flowBlockName);
    std::string preOp = OperatorExprToString(preExpr);
    depthCnt++;
    std::string curOp = OperatorExprToString(binaryExpr->rightExpr);
    subRet[curOp].emplace_back(preOp);
    preExpr = binaryExpr->rightExpr;
  } else if (SIMFlowUnaryExpression *unaryExpr =
                 dynamic_cast<SIMFlowUnaryExpression *>(flowExpr)) {
    preExpr = unaryExpr;
    if (depthCnt == 0) {
      auto name = OperatorExprToString(preExpr);
      if (subRet[name].size() == 0) {
        subRet[name] = std::vector<std::string>();
      }
    }
  } else if (SIMForeachExpression *foreachExpr =
                 dynamic_cast<SIMForeachExpression *>(flowExpr)) {
    preExpr = foreachExpr;
    if (depthCnt == 0) {
      auto name = OperatorExprToString(preExpr);
      if (subRet[name].size() == 0) {
        subRet[name] = std::vector<std::string>();
      }
    }
  } else {
    throw std::logic_error("Unsupported FlowExpression!");
  }
}

/* key: opInstance, value: preOpInstance */
std::map<std::string, std::vector<std::string>>
GetOperatorGraph(const std::string &flowBlockName, SIMFlowBlock *block) {
  std::map<std::string, std::vector<std::string>> ret;
  for (int exprId = 0; exprId < block->exprs.size(); exprId++) {
    std::map<std::string, std::vector<std::string>> subRet;
    SIMFlowExpression *preExpr = nullptr;
    int32_t depthCnt = 0;
    GetFlowExprGraph(subRet, preExpr, block->exprs[exprId], exprId, depthCnt,
                     flowBlockName);

    for (auto &[name, vec] : subRet) {
      ret[name].insert(ret[name].end(), vec.begin(), vec.end());
    }
  }
  return ret;
}

std::string SIMIRBuilder::EmitFlowFunc(SIMTranslationUnit *unit) {
  std::string ret;
  for (auto &[sym, block] : unit->flowBlocks) {
    ret +=
        StringFormat("\n\tstd::vector<uint64_t> %s(const std::vector<uint64_t> "
                     "&_instancePreId) {\n",
                     sym->name.c_str());
    auto operatorGraph = GetOperatorGraph(sym->name, block);
    for (auto &[opExpr, preOpExprVec] : operatorGraph) {
      for (auto preOpExpr : preOpExprVec) {
        ret += "\t//    " + opExpr + " <- " + preOpExpr + "\n";
      }
    }
    ret += GenRegisterInstanceCall(operatorGraph);
    ret += "\t    return ret;\n";
    ret += "\t}\n";
  }

  std::string foreachIR;
  for (auto &[name, header] : g_ForeachExprIRHeader) {
    foreachIR += header;
  }
  foreachIR += "\n";
  for (auto &[name, ir] : g_ForeachExprIR) {
    foreachIR += ir;
  }

  ret = foreachIR + ret;
  return ret;
}

std::string VisitSIMBlock(SIMIRBuilder &builder, SIMSimuBlock *block,
                          int32_t depth) {
  std::string ret;
  std::string space = "    ";
  for (int i = 0; i < depth; i++) {
    space += "    ";
  }
  for (auto *expr : block->exprs) {
    if (SIMCallExpression *callExpr = dynamic_cast<SIMCallExpression *>(expr)) {
      if (callExpr->name->name != "sleep") {
        ret += StringFormat("\t%s%s({});\n", space.c_str(),
                            callExpr->name->name.c_str());
      } else {
        ret += StringFormat("\t%susleep(%d * 1000);\n", space.c_str(),
                            callExpr->arg0);
      }
    } else if (SIMForeachExpression *foreachExpr =
                   dynamic_cast<SIMForeachExpression *>(expr)) {
      ret += StringFormat("\t%sfor(int %s = 0; %s < %d; %s++) {\n",
                          space.c_str(), "i", "i", foreachExpr->loopCnt, "i");
      ret += VisitSIMBlock(builder,
                           dynamic_cast<SIMSimuBlock *>(foreachExpr->loopBlock),
                           depth + 1);
      ret += StringFormat("\t%s}\n", space.c_str());
    }
  }
  return ret;
}

std::string SIMIRBuilder::EmitSimuFunc(SIMTranslationUnit *unit) {
  std::string ret = R"(
        void simu() {
  )";
  ret += VisitSIMBlock(*this, unit->simu, 0);
  ret += "\t}\n";
  return ret;
}

std::string SIMIRBuilder::EmitSpinLockClass(SIMTranslationUnit *unit) {
  std::string ret = R"(
        class SpinLock {
        public:
          SpinLock() : flag_(false) {}
          void lock() {
            bool expect = false;
            while (!flag_.compare_exchange_weak(expect, true)) {
              expect = false;
            }
          }
          void unlock() { flag_.store(false); }
        private:
          std::atomic<bool> flag_;
        };
        SpinLock aliveInstanceMutex;
  )";
  return ret;
}

/*
HardwareQueuePushCall IR demo:

  if (curInstanceType == Hardware::CPU) {
    Hardware_CPU_Queue.push(id);
  } else if (curInstanceType == Hardware::NPU) {
    Hardware_NPU_Queue.push(id);
  } else if (curInstanceType == Hardware::GPU) {
    Hardware_GPU_Queue.push(id);
  }
*/
std::string GenHardwareQueuePushCall(SIMTranslationUnit *unit) {
  std::string ret;

  for (int i = 0; i < unit->hardware->exprs.size(); i++) {
    if (i == 0) {
      ret += StringFormat(R"(
              if (curInstanceType == Hardware::%s) {
                Hardware_%s_Queue.push(id);)",
                          unit->hardware->exprs[i]->hardwareName->name.c_str(),
                          unit->hardware->exprs[i]->hardwareName->name.c_str());
    } else {
      ret += StringFormat(R"(
              } else if (curInstanceType == Hardware::%s) {
                Hardware_%s_Queue.push(id);)",
                          unit->hardware->exprs[i]->hardwareName->name.c_str(),
                          unit->hardware->exprs[i]->hardwareName->name.c_str());
    }
  }
  ret += "\n\t      }\n";
  return ret;
}

/*
tmpQueuePushCall IR demo:

  if (curInstanceType == Hardware::CPU) {
    tmpCPUQueue.emplace_back(successorId);
  } else if (curInstanceType == Hardware::NPU) {
    tmpNPUQueue.emplace_back(successorId);
  } else if (curInstanceType == Hardware::GPU) {
    tmpGPUQueue.emplace_back(successorId);
  }
*/
std::string GenTmpQueuePushCall(SIMTranslationUnit *unit,
                                const std::string &idSymbolName) {
  std::string ret;

  for (int i = 0; i < unit->hardware->exprs.size(); i++) {
    if (i == 0) {
      ret += StringFormat(R"(
              if (curInstanceType == Hardware::%s) {
                tmp%sQueue.emplace_back(%s);)",
                          unit->hardware->exprs[i]->hardwareName->name.c_str(),
                          unit->hardware->exprs[i]->hardwareName->name.c_str(),
                          idSymbolName.c_str());
    } else {
      ret += StringFormat(R"(
              } else if (curInstanceType == Hardware::%s) {
                tmp%sQueue.emplace_back(%s);)",
                          unit->hardware->exprs[i]->hardwareName->name.c_str(),
                          unit->hardware->exprs[i]->hardwareName->name.c_str(),
                          idSymbolName.c_str());
    }
  }
  ret += "\n\t      }\n";
  return ret;
}

std::string SIMIRBuilder::EmitSimpleScheduler(SIMTranslationUnit *unit) {
  std::string ret = R"(
        void SimpleScheduler(std::vector<uint64_t> &instanceHeader) {

          for (auto id : instanceHeader) {
              auto curInstanceType = operatorToHardwareTime.at(instanceToOperator.at(id)).first;
      )";

  ret += GenHardwareQueuePushCall(unit);

  ret += R"(
              aliveInstanceId.at(id) = 1;
          }
        }
  )";

  return ret;
}

std::string SIMIRBuilder::EmitGreedyScheduler(SIMTranslationUnit *unit) {
  std::string ret = R"(
        void GreedyScheduler(std::vector<uint64_t> &instanceHeader) {
          std::vector<uint64_t> tmpCPUQueue, tmpNPUQueue, tmpGPUQueue;
          for (auto id : instanceHeader) {
            auto curInstanceType = operatorToHardwareTime.at(instanceToOperator.at(id)).first;

  )";

  ret += GenTmpQueuePushCall(unit, "id");

  ret += R"(
            aliveInstanceId.at(id) = 1;
          }

          int headerId = 0;
          while (headerId < instanceHeader.size()) {
            auto curType = operatorToHardwareTime
                              .at(instanceToOperator.at(instanceHeader[headerId]))
                              .first;
            if (hardwareCnt.at(curType) > 1) {
              headerId++;
              continue;
            }
            for (auto successorId : instancePostId[instanceHeader[headerId]]) {
              bool allPreIdSameType = true;
              for (auto preId : instancePreId.at(successorId)) {
                if (aliveInstanceId.count(preId) != 0) {
                  if (operatorToHardwareTime.at(instanceToOperator.at(preId)).first !=
                      curType) {
                    allPreIdSameType = false;
                    break;
                  }
                }
              }
              if (allPreIdSameType) {
                instanceHeader.emplace_back(successorId);
                aliveInstanceId.at(successorId) = 1;
                auto curInstanceType =
                    operatorToHardwareTime.at(instanceToOperator.at(successorId)).first;

  )";
  ret += GenTmpQueuePushCall(unit, "successorId");
  ret += R"(
              }
            }
            headerId++;
          }

  )";

  for (int i = 0; i < unit->hardware->exprs.size(); i++) {
    std::string templateStr = R"(
          for (auto id : tmp%sQueue) {
            Hardware_%s_Queue.push(id);
          }
    )";
    ret += StringFormat(templateStr.c_str(),
                        unit->hardware->exprs[i]->hardwareName->name.c_str(),
                        unit->hardware->exprs[i]->hardwareName->name.c_str());
  }

  ret += "\t}\n";
  return ret;
}

std::string SIMIRBuilder::EmitMainFunc(SIMTranslationUnit *unit) {
  std::string ret = R"(
        int main() {
            std::thread simuThread(simu);
            std::thread instanceExec(InstanceExecuteService);
   )";
  for (auto *expr : unit->hardware->exprs) {
    for (int32_t deviceId = 0; deviceId < expr->hardwareCnt; deviceId++) {
      ret += StringFormat("\t    std::thread %sThread_%d(%sExecute, %d);\n",
                          expr->hardwareName->name.c_str(), deviceId,
                          expr->hardwareName->name.c_str(), deviceId);
    }
  }
  ret += R"(

            gettimeofday(&initTime, NULL);

            simuThread.join();
            simuDone = 1;
            instanceExec.join();
            schedulerDone = 1;
    )";
  for (auto *expr : unit->hardware->exprs) {
    for (int32_t deviceId = 0; deviceId < expr->hardwareCnt; deviceId++) {
      ret += StringFormat("\t    %sThread_%d.join();\n",
                          expr->hardwareName->name.c_str(), deviceId);
    }
  }
  ret += "\t    gettimeofday(&endTime, NULL);\n";
  ret += R"(
            double totalTime = ((endTime.tv_sec * 1000000ULL + endTime.tv_usec) -
                          (initTime.tv_sec * 1000000ULL + initTime.tv_usec)) /
                          (double)1000000;)";
  ret += R"(
            printf("\n\nHARDWARE\tBUSY_TIME\tIDLE_TIME\tUSAGE\n\n");
  )";
  for (auto *expr : unit->hardware->exprs) {
    for (int32_t deviceId = 0; deviceId < expr->hardwareCnt; deviceId++) {
      ret += R"(
            printf("%s\t\t%.3lf\t\t%.3lf\t\t%.1lf%%\n", )";
      ret += StringFormat(
          R"("%s_%d", %s_TheoreticalTime[%d], totalTime - %s_TheoreticalTime[%d],
                          %s_TheoreticalTime[%d] * 100 / totalTime);
      )",
          expr->hardwareName->name.c_str(), deviceId,
          expr->hardwareName->name.c_str(), deviceId,
          expr->hardwareName->name.c_str(), deviceId,
          expr->hardwareName->name.c_str(), deviceId);
    }
  }
  ret += R"(
            std::cout << std::endl;
            std::cout << "Total : " << totalTime << " seconds" << std::endl;
        })";
  return ret;
}

} // namespace XPUSchedulerSimulator