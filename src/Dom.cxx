//#include <xercesc/util/PlatformUtils.hpp>
//#include <xercesc/util/XMLString.hpp>
//#include <xercesc/dom/DOM.hpp>
#include "xmlBase/xml_builder.h"
#include "xmlBase/rapidxml.hpp"
#include "optimizers/Dom.h"

//namespace {
  //using XERCES_CPP_NAMESPACE_QUALIFIER XMLString;

//    class XStr {
//    public:
//       XStr(const char * const toTranscode) {
//          m_unicodeForm = XMLString::transcode(toTranscode);
//       }
//       XStr(const std::string & toTranscode) {
//          m_unicodeForm = XMLString::transcode(toTranscode.c_str());
//       }
//       ~XStr() {
//          XMLString::release(&m_unicodeForm);
//       }
//       const XMLCh * unicodeForm() const {
//          return m_unicodeForm;
//       }
//    private:
//       XMLCh * m_unicodeForm;
//    };
// }

namespace optimizers {
//   XERCES_CPP_NAMESPACE_USE
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementation;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMImplementationRegistry;
  //using XERCES_CPP_NAMESPACE_QUALIFIER rapidxml::xml_node<>;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMNode;

   rapidxml::xml_document<> * Dom::createDocument() {
     //DOMImplementation * impl = DOMImplementationRegistry::
     //    getDOMImplementation(::XStr("Core").unicodeForm());
      rapidxml::xml_document<> * doc;
      return doc;
   }

   rapidxml::xml_node<> * Dom::createElement(rapidxml::xml_document<> * doc, 
                                   const std::string & name) {
     rapidxml::xml_node<> * elt = doc->allocate_node(rapidxml::node_element, doc->allocate_string(name.c_str()));
      // new xml_builder::XmlElementBuilder Xmlbuilder(doc);
      // rapidxml::xml_node<> * elt = Xmlbuilder->addElement(name));
      return elt;
   }

  void Dom::appendChild(rapidxml::xml_node<> * parent, rapidxml::xml_node<> * child) {
      parent->append_node(child);
   }

   // void Dom::appendChild(rapidxml::xml_node<> * parent, rapidxml::xml_node<> * child) {
   //    parent->appendChild(reinterpret_cast<DOMNode *>(child));
   // }
}
