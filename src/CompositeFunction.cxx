/** 
 * @file CompositeFunction.cxx
 * @brief CompositeFunction class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include <vector>
#include <string>
#include <cmath>
#include <cassert>
#include "optimizers/CompositeFunction.h"

namespace optimizers {

CompositeFunction::CompositeFunction(const CompositeFunction &rhs) 
   : Function(rhs) {
// Make a deep copy since Functions are stored as pointers.

   Function *afuncptr = rhs.m_a->clone();
   m_a = afuncptr;

   Function *bfuncptr = rhs.m_b->clone();
   m_b = bfuncptr;

   syncParams();
}

void CompositeFunction::setParam(const Parameter &param, 
                                 const std::string &funcName) {
   assert(funcName == m_a->getName() || funcName == m_b->getName());

   if (m_a->getName() == funcName) {
      m_a->setParam(param);
   } else {
      m_b->setParam(param);
   }
   syncParams();
}

const Parameter & 
CompositeFunction::getParam(const std::string &paramName,
                            const std::string &funcName) const {
   assert(funcName == m_a->getName() || funcName == m_b->getName());

   if (m_a->getName() == funcName) {
      return m_a->getParam(paramName);
   } else {
      return m_b->getParam(paramName);
   }
}

void CompositeFunction::syncParams() {
   m_parameter.clear();

   std::vector<Parameter> params;

   m_a->getParams(params);
   for (unsigned int i = 0; i < params.size(); i++) 
      m_parameter.push_back(params[i]);

   m_b->getParams(params);
   for (unsigned int i = 0; i < params.size(); i++) 
      m_parameter.push_back(params[i]);
}

} // namespace optimizers
