//#include <xercesc/util/PlatformUtils.hpp>
//#include <xercesc/util/XMLString.hpp>
//#include <xercesc/dom/DOM.hpp>
#include "xmlbase/xml_builder.hpp"

#include "optimizers/Dom.h"

namespace {
  //using XERCES_CPP_NAMESPACE_QUALIFIER XMLString;

   class XStr {
   public:
      XStr(const char * const toTranscode) {
         m_unicodeForm = XMLString::transcode(toTranscode);
      }
      XStr(const std::string & toTranscode) {
         m_unicodeForm = XMLString::transcode(toTranscode.c_str());
      }
      ~XStr() {
         XMLString::release(&m_unicodeForm);
      }
      const XMLCh * unicodeForm() const {
         return m_unicodeForm;
      }
   private:
      XMLCh * m_unicodeForm;
   };
}

namespace optimizers {
//   XERCES_CPP_NAMESPACE_USE
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementationRegistry;
  //using XERCES_CPP_NAMESPACE_QUALIFIER xmlbase::xml_node<>;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMNode;

   xmlbase::xml_document<> * Dom::createDocument() {
     //DOMImplementation * impl = DOMImplementationRegistry::
     //    getDOMImplementation(::XStr("Core").unicodeForm());
      xmlbase::xml_document<> * doc;
      return doc;
   }

   xmlbase::xml_node<> * Dom::createElement(xmlbase::xml_document<> * doc, 
                                   const std::string & name) {
      xmlbase::xml_node<> * elt = doc->createElement(::XStr(name).unicodeForm());
      return elt;
   }

   void Dom::appendChild(DOMNode * parent, xmlbase::xml_node<> * child) {
      parent->appendChild(reinterpret_cast<DOMNode *>(child));
   }

   void Dom::appendChild(xmlbase::xml_node<> * parent, xmlbase::xml_node<> * child) {
      parent->appendChild(reinterpret_cast<DOMNode *>(child));
   }
}
