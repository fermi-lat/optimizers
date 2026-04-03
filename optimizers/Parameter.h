/** 
 * @file Parameter.h
 * @brief Declaration of Parameter class
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef optimizers_Parameter_h
#define optimizers_Parameter_h

#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

// RapidXML-based XML framework (replaces Xerces-C)
#include "xmlBase/rapidxml.hpp"

namespace optimizers {

class Function;

/** 
 * @class Parameter
 *
 * @brief Model parameters are identified by a name with flags to
 * indicate if it's free and with upper and lower bounds.
 *
 * The true value of the Parameter is used in the Function
 * calculation.  Only the (apparent) value is intended to accessible
 * through the value accessor methods of the Function class.
 *
 */
class Parameter {

   friend class Function;

public:
   /// Default constructor
   Parameter() 
      : m_name("")
      , m_value(0)
      , m_minValue(-std::numeric_limits<double>::infinity())
      , m_maxValue(std::numeric_limits<double>::infinity())
      , m_free(true)
      , m_scale(1.)
      , m_error(0)
      , m_alwaysFixed(false)
      , m_par_ref(nullptr)
      , m_log_prior(nullptr) 
   {}

   /// @param name The name of the Parameter
   /// @param value The (scaled) value of the Parameter
   /// @param minValue Parameter value lower bound
   /// @param maxValue Parameter value upper bound
   /// @param isFree true if the Parameter value is allowed to vary in a fit
   /// @param error estimated error on Parameter value.
   Parameter(const std::string& name, double value, double minValue,
             double maxValue, bool isFree = true, double error = 0) 
      : m_name(name)
      , m_value(value)
      , m_minValue(minValue)
      , m_maxValue(maxValue)
      , m_free(isFree)
      , m_scale(1.)
      , m_error(error)
      , m_alwaysFixed(false)
      , m_par_ref(nullptr)
      , m_log_prior(nullptr) 
   {}

   Parameter(const std::string& name, double value, bool isFree = true)
      : m_name(name)
      , m_value(value)
      , m_minValue(-std::numeric_limits<double>::infinity())
      , m_maxValue(std::numeric_limits<double>::infinity())
      , m_free(isFree)
      , m_scale(1.)
      , m_error(0)
      , m_alwaysFixed(false)
      , m_par_ref(nullptr)
      , m_log_prior(nullptr) 
   {}

   Parameter(const Parameter& other);

   Parameter& operator=(const Parameter& rhs);

   virtual ~Parameter() noexcept = default;

   // ==================== Name Access ====================
   
   virtual void setName(const std::string& paramName) {
      m_name = paramName;
      if (m_par_ref) {
         m_par_ref->setName(paramName);
      }
   }

   [[nodiscard]] const std::string& getName() const noexcept {
      return m_name;
   }
   
   // ==================== Value Access ====================
   
   virtual void setValue(double value);

   [[nodiscard]] double getValue() const noexcept {
      return m_value;
   }
   
   // ==================== Scale Access ====================
   
   virtual void setScale(double scale) {
      m_scale = scale;
      if (m_par_ref) {
         m_par_ref->setScale(scale);
      }
   }

   [[nodiscard]] double getScale() const noexcept {
      return m_scale;
   }

   // ==================== True Value Access ====================

   virtual void setTrueValue(double trueValue);

   [[nodiscard]] double getTrueValue() const noexcept {
      return m_value * m_scale;
   }

   // ==================== Bounds Access ====================

   virtual void setBounds(double minValue, double maxValue);

   virtual void setBounds(const std::pair<double, double>& boundValues) {
      setBounds(boundValues.first, boundValues.second);
   }

   [[nodiscard]] std::pair<double, double> getBounds() const noexcept;

   // ==================== Free Flag Access ====================

   virtual void setFree(bool free) {
      if (m_alwaysFixed) {
         m_free = false;
      } else {
         m_free = free;
      }
      if (m_par_ref) {
         m_par_ref->setFree(free);
      }
   }

   [[nodiscard]] bool isFree() const noexcept {
      return m_free;
   }

   virtual void setAlwaysFixed(bool flag) {
      m_alwaysFixed = flag;
      if (m_par_ref) {
         m_par_ref->setAlwaysFixed(flag);
      }
   }

   [[nodiscard]] bool alwaysFixed() const noexcept {
      return m_alwaysFixed;
   }

   // ==================== Error Access ====================

   virtual void setError(double error) {
      m_error = error;
      if (m_par_ref) {
         m_par_ref->setError(error);
      }
   }

   [[nodiscard]] double error() const noexcept {
      return m_error;
   }

   // ==================== XML Serialization (RapidXML) ====================

#ifndef SWIG
   /// Extract data from an xml parameter element defined using the
   /// FunctionModels.dtd.
   /// @param elt Pointer to RapidXML node containing parameter data
   void extractDomData(const rapidxml::xml_node<>* elt);

   /// Add a parameter DomElement that contains the current data
   /// member values.
   /// @param doc Pointer to RapidXML document for memory allocation
   /// @return Pointer to newly created parameter node
   [[nodiscard]] rapidxml::xml_node<>* createDomElement(rapidxml::xml_document<>* doc) const;
#endif // SWIG

   // ==================== Parameter Reference ====================

   void setParRef(Parameter* par) {
      m_par_ref = par;
      if (par) {
         m_name = par->m_name;
         m_value = par->m_value;
         m_minValue = par->m_minValue;
         m_maxValue = par->m_maxValue;
         m_free = par->m_free;
         m_scale = par->m_scale;
         m_error = par->m_error;
         m_alwaysFixed = par->m_alwaysFixed;
      }
   }

   void setDataValues(const Parameter& par) {
      m_name = par.m_name;
      m_value = par.m_value;
      m_minValue = par.m_minValue;
      m_maxValue = par.m_maxValue;
      m_free = par.m_free;
      m_scale = par.m_scale;
      m_error = par.m_error;
      m_alwaysFixed = par.m_alwaysFixed;
   }
     
   [[nodiscard]] const Parameter* getParRef() const noexcept {
      return m_par_ref;
   } 
  
   // ==================== Prior Function ====================

   void setPrior(Function& log_prior);

   [[nodiscard]] bool has_prior() const noexcept { 
      return m_log_prior != nullptr; 
   }

   Function* removePrior();

   [[nodiscard]] double log_prior_value() const;

   [[nodiscard]] double log_prior_deriv() const;

   [[nodiscard]] Function& log_prior() {
      return *m_log_prior;
   }

   [[nodiscard]] const Function& log_prior() const {
      return *m_log_prior;
   }

protected:
   std::string m_name;
   double m_value;
   double m_minValue;
   double m_maxValue;

   /// flag to indicate free or fixed
   bool m_free;

   double m_scale;

   /// estimated error on value
   double m_error;

   /// If true, then m_free is always false and cannot be changed.
   bool m_alwaysFixed;

   /// pointer to underlying Parameter object for use by composite Function
   /// classes. This will not be deleted by this class.
   Parameter* m_par_ref;

   /// Pointer to prior function (the log of the 1D PDF).  This will
   /// not be deleted by this class.
   Function* m_log_prior;
};

} // namespace optimizers

#endif // optimizers_Parameter_h
