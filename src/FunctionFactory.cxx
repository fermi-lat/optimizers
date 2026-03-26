/** 
 * @file FunctionFactory.cxx 
 * @brief Use the prototype pattern for supplying customized Function
 * objects, the parameters of which are specified in an xml file.
 * 
 * @author J. Chiang
 *
 * $Header$
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

// #include <xercesc/util/PlatformUtils.hpp>
// #include <xercesc/util/XMLString.hpp>
// #include <xercesc/dom/DOM.hpp>

// #include "xmlBase/Dom.h"
// #include "xmlBase/XmlParser.h"

#include "optimizers/Dom.h"
#include "optimizers/Exception.h"
#include "optimizers/FunctionFactory.h"
#include "optimizers/Gaussian.h"

#include "PowerLaw.h"
#include "BrokenPowerLaw.h"
#include "AbsEdge.h"
#include "ConstantValue.h"

namespace optimizers {

FunctionFactory::FunctionFactory() {
// Some standard functions.
   addFunc("PowerLaw", new PowerLaw(), false);
   addFunc("BrokenPowerLaw", new BrokenPowerLaw(), false);
   addFunc("Gaussian", new Gaussian(), false);
   addFunc("AbsEdge", new AbsEdge(), false);
   addFunc("ConstantValue", new ConstantValue(), false);
}

FunctionFactory::~FunctionFactory() {
   std::map<std::string, Function *>::iterator it = m_prototypes.begin();
   for (; it != m_prototypes.end(); it++) {
      delete it->second;
   }
}

void FunctionFactory::addFunc(const std::string & name,
                              optimizers::Function * func,
                              bool fromClone) {
   if (m_prototypes.count(name)) {
      std::ostringstream message;
      message << "FunctionFactory::addFunc: A Function named "
              << name << " already exists.";
      throw std::runtime_error(message.str());
   }
   if (fromClone) {
      m_prototypes[name] = func->clone();
   } else {
      m_prototypes[name] = func;
   }
}

void FunctionFactory::addFunc(optimizers::Function * func,
                              bool fromClone) {
   addFunc(func->genericName(), func, fromClone);
}

Function *FunctionFactory::create(const std::string &name) {
   if (!m_prototypes.count(name)) {
      std::ostringstream errorMessage;
      errorMessage << "FunctionFactory::create: "
                   << "Cannot create Function named "
                   << name << ".\n";
      throw Exception(errorMessage.str());
   }
   return m_prototypes[name]->clone();
}

void FunctionFactory::getFunctionNames(std::vector<std::string> &funcNames) {
   funcNames.clear();
   std::map<std::string, Function *>::const_iterator it = m_prototypes.begin();
   for ( ; it != m_prototypes.end(); it++) {
      funcNames.push_back(it->first);
   }
}

void FunctionFactory::readXml(const std::string &xmlFile) {
   xml_framework::SafeXmlParser *  parser = new xml_framework::SafeXmlParser();
   
   rapidxml::xml_document<> * doc = parser->parse(xmlFile.c_str());

   if (doc == 0) { // xml file not parsed successfully
      std::string errorMessage = "FunctionFactory::readXml:\nInput xml file, "
         + xmlFile + " not parsed successfully.";
      throw Exception(errorMessage);
   }

// Direct Xerces API call.
   rapidxml::xml_node<> * function_library = doc->getDocumentElement();
   // if (!xmlBase::Dom::checkTagName(function_library, "function_library")) {
   //    throw Exception(std::string("FunctionFactory::readXml:\n")
   //                    + "function_library not found in "
   //                    + xmlFile);
   // }

// Loop through function child elements, and add each as a Function
// object to the prototype factory.
   std::vector<rapidxml::xml_node<> *> funcs;
   std::string paramstring = "function";
   funcs = parser->getChildren(function_library, paramstring.c_string());

   std::vector<rapidxml::xml_node<> *>::const_iterator funcIt = funcs.begin();
   for ( ; funcIt != funcs.end(); funcIt++) {

// Get the type of this function, which should be an existing 
// (generic) Function in the factory.
     std::string type = parser->getAttribute<std::string>(*funcIt, "type").value();
      Function *funcObj;
      try {
         funcObj = create(type);
      } catch (Exception &eObj) {
         std::cerr << "FunctionFactory::readXml: "
                   << "Failed to create Function object "
                   << type << std::endl;
         throw;
      }

// Set the name of this function prototype.
      std::string name = parser->getAttribute<std::string>(*funcIt, "name").value();
// Use the type attribute as the name for use by writeXml as the type
// information.
      funcObj->setName(type);

// Fetch the parameter elements and set the Parameter data members.
      std::vector<rapidxml::xml_node<> *> params;
      paramstring = "parameter";
      params = parser->getChildren(*funcIt, paramstring).value();
   
      std::vector<rapidxml::xml_node<> *>::const_iterator paramIt = params.begin();
      for (; paramIt != params.end(); paramIt++) {
	 std::string paramName = parser->getAttribute<std::string>(*paramIt, "name");
         funcObj->parameter(paramName).extractDomData(*paramIt);
      }
      addFunc(name, funcObj, false);
   }
   delete parser;
}

void FunctionFactory::writeXml(const std::string &xmlFile) {
   rapidxml::xml_document<> * doc = new rapidxml::xml_document<>;

   xml_builder::XmlElementBuilder * funcLibWriter = new xml_builder::XmlElementBuilder();
   rapidxml::xml_node<> * funcLib = new rapidxml::xml_node<>;
   funcLibWriter->XmlElementBuilder(doc, funcLib);
   rapidxml::xml_node<> * funcLib = xmlwriter->createElement(doc, "function_library");
   xmlBase::Dom::addAttribute(funcLib, "title", "prototype Functions");

// Loop over the Function prototypes, keeping only the derived prototypes.
   std::map<std::string, Function *>::iterator funcIt = m_prototypes.begin();
   for ( ; funcIt != m_prototypes.end(); funcIt++) {
      rapidxml::xml_node<> * funcElt = Dom::createElement(doc, "function");
      std::string name = funcIt->first;
      xmlBase::Dom::addAttribute(funcElt, "name", name.c_str());
      std::string type = funcIt->second->getName();
      if (type == std::string("")) {
// Skip this Function since a lack of type implies a base prototype.
         continue;
      } else {
// Use the generic name of the Function object as the type attribute.
         xmlBase::Dom::addAttribute(funcElt, "type", 
                                funcIt->second->genericName().c_str());
      }

      funcIt->second->appendParamDomElements(doc, funcElt);

      funcLib->appendChild(funcElt);
   }

// Write the XML file using the static function.
   std::ofstream outFile(xmlFile.c_str());
   outFile << "<?xml version='1.0' standalone='no'?>\n"
           << "<!DOCTYPE function_library SYSTEM "
           << "\"$(OPTIMIZERSXMLPATH)/FunctionModels.dtd\" >\n";
   xmlBase::Dom::prettyPrintElement(funcLib, outFile, "");
   doc->release();
}

} // namespace optimizers
