#ifndef optimizers_Dom_h
#define optimizers_Dom_h

#include <string>
#include "xmlBase/rapidxml.hpp"
//#include <xercesc/util/XercesDefs.hpp>

//XERCES_CPP_NAMESPACE_BEGIN
//class DOMElement;
//class DOMDocument;
//class DOMNode;
//XERCES_CPP_NAMESPACE_END

namespace optimizers {

  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument;
  //using XERCES_CPP_NAMESPACE_QUALIFIER rapidxml::xml_node<>;
  //using XERCES_CPP_NAMESPACE_QUALIFIER DOMNode;

class Dom {

public:

   static rapidxml::xml_document<> * createDocument();

   static rapidxml::xml_node<> * createElement(rapidxml::xml_document<> * doc, 
                                     const std::string & name);

   static void appendChild(rapidxml::xml_node<> * parent, rapidxml::xml_node<> * child);

};

} // namespace optimizers

#endif // optimizers_Dom_h
