/** 
 * @file Parameter.cpp
 * @brief Parameter class implementation
 * @author J. Chiang
 *
 * $Header$
 */

#include <cstdlib>
#include <cmath>
#include <cstring>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <iomanip>

// RapidXML-based XML framework (replaces Xerces-C)
#include "xmlBase/rapidxml.hpp"

#include "optimizers/dArg.h"
#include "optimizers/Function.h"
#include "optimizers/OutOfBounds.h"
#include "optimizers/Parameter.h"

namespace optimizers {

// ============================================================================
// Helper Functions for RapidXML Attribute Access
// ============================================================================

namespace {

/// Get attribute value as string, returns empty string if not found
[[nodiscard]] std::string getAttribute(const rapidxml::xml_node<>* node, 
                                       const char* attrName) {
    if (!node) return "";
    
    auto* attr = node->first_attribute(attrName);
    if (attr && attr->value()) {
        return std::string(attr->value(), attr->value_size());
    }
    return "";
}

/// Check if an attribute exists
[[nodiscard]] bool hasAttribute(const rapidxml::xml_node<>* node, 
                                const char* attrName) {
    if (!node) return false;
    return node->first_attribute(attrName) != nullptr;
}

/// Convert string to double with fallback
[[nodiscard]] double toDouble(const std::string& str, double defaultValue = 0.0) {
    if (str.empty()) return defaultValue;
    
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

/// Convert double to string with specified precision
[[nodiscard]] std::string doubleToString(double value, int precision = 10) {
    std::ostringstream oss;
    oss << std::setprecision(precision) << value;
    return oss.str();
}

/// Convert bool to string
[[nodiscard]] const char* boolToString(bool value) {
    return value ? "true" : "false";
}

/// Add an attribute to a RapidXML node
void addAttribute(rapidxml::xml_document<>* doc, 
                  rapidxml::xml_node<>* node,
                  const char* name, 
                  const std::string& value) {
    char* allocName = doc->allocate_string(name);
    char* allocValue = doc->allocate_string(value.c_str(), value.size() + 1);
    node->append_attribute(doc->allocate_attribute(allocName, allocValue));
}

void addAttribute(rapidxml::xml_document<>* doc,
                  rapidxml::xml_node<>* node,
                  const char* name,
                  double value,
                  int precision = 10) {
    addAttribute(doc, node, name, doubleToString(value, precision));
}

void addAttribute(rapidxml::xml_document<>* doc,
                  rapidxml::xml_node<>* node,
                  const char* name,
                  bool value) {
    addAttribute(doc, node, name, std::string(boolToString(value)));
}

} // anonymous namespace

// ============================================================================
// Copy Constructor and Assignment
// ============================================================================

Parameter::Parameter(const Parameter& other) 
   : m_name(other.m_name)
   , m_value(other.m_value)
   , m_minValue(other.m_minValue)
   , m_maxValue(other.m_maxValue)
   , m_free(other.m_free)
   , m_scale(other.m_scale)
   , m_error(other.m_error)
   , m_alwaysFixed(other.m_alwaysFixed)
   , m_par_ref(other.m_par_ref)
   , m_log_prior(other.m_log_prior)
{
}

Parameter& Parameter::operator=(const Parameter& rhs) {
   if (this != &rhs) {
      m_name = rhs.m_name;
      m_value = rhs.m_value;
      m_minValue = rhs.m_minValue;
      m_maxValue = rhs.m_maxValue;
      m_free = rhs.m_free;
      m_scale = rhs.m_scale;
      m_error = rhs.m_error;
      m_alwaysFixed = rhs.m_alwaysFixed;
      m_par_ref = rhs.m_par_ref;
      m_log_prior = rhs.m_log_prior;
   }
   return *this;
}

// ============================================================================
// Value and Bounds Management
// ============================================================================

void Parameter::setValue(double value) {
   static constexpr double tol = 1e-8;
   
   // Check if value is within tolerance of min bound
   if (!std::isinf(m_minValue) && m_minValue != 0 && 
       std::fabs((value - m_minValue) / m_minValue) < tol) {
      m_value = m_minValue;
   } 
   // Check if value is within tolerance of max bound
   else if (!std::isinf(m_maxValue) && m_maxValue != 0 && 
            std::fabs((value - m_maxValue) / m_maxValue) < tol) {
      m_value = m_maxValue;
   } 
   // Check if value is within bounds
   else if (value >= m_minValue && value <= m_maxValue) {
      m_value = value;
   } 
   // Special case: no bounds set (Minuit interface)
   else if (m_minValue == 0. && m_maxValue == 0.) {
      m_value = value;
   } 
   else {
      throw OutOfBounds(
         "Attempt to set the value outside of existing bounds.", 
         value, m_minValue, m_maxValue, 
         static_cast<int>(OutOfBounds::VALUE_ERROR));
   }
   
   if (m_par_ref) {
      m_par_ref->setValue(value);
   }
}

void Parameter::setTrueValue(double trueValue) {
   double value = trueValue / m_scale;
   setValue(value);
   if (m_par_ref) {
      m_par_ref->setValue(value);
   }
}

void Parameter::setBounds(double minValue, double maxValue) {
   if (m_value >= minValue && m_value <= maxValue) {
      m_minValue = minValue;
      m_maxValue = maxValue;
   } else if (minValue == 0. && maxValue == 0.) {
      // Minuit interface: parameter without limits
      m_minValue = minValue;
      m_maxValue = maxValue;     
   } else {
      throw OutOfBounds(
         "Attempt to set bounds that exclude the existing value.", 
         m_value, minValue, maxValue, 
         static_cast<int>(OutOfBounds::BOUNDS_ERROR));
   }
   
   if (m_par_ref) {
      m_par_ref->setBounds(minValue, maxValue);
   }
}

std::pair<double, double> Parameter::getBounds() const noexcept {
   return {m_minValue, m_maxValue};
}

// ============================================================================
// XML Serialization (RapidXML-based)
// ============================================================================

void Parameter::extractDomData(const rapidxml::xml_node<>* elt) {
   if (!elt) {
      throw std::invalid_argument("Parameter::extractDomData: null element");
   }

   // Extract name attribute
   m_name = getAttribute(elt, "name");
   
   // Extract value attribute
   m_value = toDouble(getAttribute(elt, "value"), 0.0);
   
   // Extract min/max bounds
   m_minValue = toDouble(getAttribute(elt, "min"), 0.0);
   m_maxValue = toDouble(getAttribute(elt, "max"), 0.0);
   
   // Validate bounds
   if (m_minValue == 0. && m_maxValue == 0.) {
      // Minuit interface: parameter is given without limits,
      // which is fine, don't throw out-of-bounds exception;
      // Minuit2 will check for this case in the same manner.
   } else if (m_value < m_minValue || m_value > m_maxValue) {
      std::ostringstream message;
      message << "Parameter::extractDomData:\n"
              << "In the XML description of parameter '" << m_name << "', "
              << "An attempt has been made to set the parameter value ("
              << m_value << ") outside of the specified bounds ["
              << m_minValue << ", " << m_maxValue << "].";
      throw std::out_of_range(message.str());
   }
   
   // Extract free attribute
   std::string freeStr = getAttribute(elt, "free");
   // Convert to lowercase for comparison
   std::transform(freeStr.begin(), freeStr.end(), freeStr.begin(),
                  [](unsigned char c) { return std::tolower(c); });
   m_free = (freeStr == "true" || freeStr == "1");
   
   // Extract scale attribute
   m_scale = toDouble(getAttribute(elt, "scale"), 1.0);
   
   // Extract optional error attribute
   if (hasAttribute(elt, "error")) {
      m_error = toDouble(getAttribute(elt, "error"), 0.0);
   } else {
      m_error = 0;
   }
   
   // Propagate to referenced parameter if exists
   if (m_par_ref) {
      m_par_ref->extractDomData(elt);
   }
}

rapidxml::xml_node<>* Parameter::createDomElement(rapidxml::xml_document<>* doc) const {
   if (!doc) {
      throw std::invalid_argument("Parameter::createDomElement: null document");
   }

   // Create the parameter element
   char* elemName = doc->allocate_string("parameter");
   auto* paramElt = doc->allocate_node(rapidxml::node_element, elemName);

   // Add the appropriate attributes
   addAttribute(doc, paramElt, "name", m_name);
   addAttribute(doc, paramElt, "value", m_value, 10);
   addAttribute(doc, paramElt, "min", m_minValue, 10);
   addAttribute(doc, paramElt, "max", m_maxValue, 10);
   addAttribute(doc, paramElt, "free", m_free);
   addAttribute(doc, paramElt, "scale", m_scale, 10);
   
   // Only add error attribute if it's greater than 0
   if (m_error > 0) {
      addAttribute(doc, paramElt, "error", m_error, 10);
   }

   return paramElt;
}

// ============================================================================
// Prior Function Methods
// ============================================================================

void Parameter::setPrior(Function& log_prior) {
   m_log_prior = &log_prior;
}

Function* Parameter::removePrior() {
   Function* log_prior = m_log_prior;
   m_log_prior = nullptr;
   return log_prior;
}

double Parameter::log_prior_value() const {
   if (!m_log_prior) {
      return 0;
   }
   dArg x(m_value);
   return m_log_prior->operator()(x);
}

double Parameter::log_prior_deriv() const {
   if (!m_log_prior) {
      return 0;
   }
   dArg x(m_value);
   return m_log_prior->derivative(x);
}

} // namespace optimizers
