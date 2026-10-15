/** 
 * @file FunctionFactory.cpp 
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
#include <algorithm>

// C++17 RapidXML-based XML Framework (replaces Xerces-C)
#include "xmlBase/rapidxml.hpp"
#include "xmlBase/rapidxml_error_framework.hpp"
#include "xmlBase/safe_xml_parser.hpp"
#include "xmlBase/xml_builder.h"
#include "xmlBase/xml_printer.hpp"
#include "xmlBase/xml_result.hpp"

#include "optimizers/Exception.h"
#include "optimizers/FunctionFactory.h"
#include "optimizers/Gaussian.h"

#include "PowerLaw.h"
#include "BrokenPowerLaw.h"
#include "AbsEdge.h"
#include "ConstantValue.h"

namespace optimizers {

// ============================================================================
// Construction / Destruction
// ============================================================================

FunctionFactory::FunctionFactory() {
    // Register standard function prototypes
    addFunc("PowerLaw", new PowerLaw(), false);
    addFunc("BrokenPowerLaw", new BrokenPowerLaw(), false);
    addFunc("Gaussian", new Gaussian(), false);
    addFunc("AbsEdge", new AbsEdge(), false);
    addFunc("ConstantValue", new ConstantValue(), false);
}

FunctionFactory::~FunctionFactory() {
    for (auto& [name, func] : m_prototypes) {
        delete func;
    }
    m_prototypes.clear();
}

// ============================================================================
// Function Registration
// ============================================================================

void FunctionFactory::addFunc(const std::string& name,
                              Function* func,
                              bool fromClone) {
    if (m_prototypes.count(name)) {
        std::ostringstream message;
        message << "FunctionFactory::addFunc: A Function named '"
                << name << "' already exists.";
        throw std::runtime_error(message.str());
    }
    
    if (fromClone) {
        m_prototypes[name] = func->clone();
    } else {
        m_prototypes[name] = func;
    }
}

void FunctionFactory::addFunc(Function* func, bool fromClone) {
    addFunc(func->genericName(), func, fromClone);
}

// ============================================================================
// Function Creation
// ============================================================================

Function* FunctionFactory::create(const std::string& name) {
    auto it = m_prototypes.find(name);
    if (it == m_prototypes.end()) {
        std::ostringstream errorMessage;
        errorMessage << "FunctionFactory::create: "
                     << "Cannot create Function named '"
                     << name << "'.\n";
        throw Exception(errorMessage.str());
    }
    return it->second->clone();
}

// ============================================================================
// Query Methods
// ============================================================================

void FunctionFactory::getFunctionNames(std::vector<std::string>& funcNames) const {
    funcNames.clear();
    funcNames.reserve(m_prototypes.size());
    for (const auto& [name, func] : m_prototypes) {
        funcNames.push_back(name);
    }
}

std::vector<std::string> FunctionFactory::getFunctionNames() const {
    std::vector<std::string> funcNames;
    funcNames.reserve(m_prototypes.size());
    for (const auto& [name, func] : m_prototypes) {
        funcNames.push_back(name);
    }
    return funcNames;
}

bool FunctionFactory::hasFunction(const std::string& name) const noexcept {
    return m_prototypes.count(name) > 0;
}

std::size_t FunctionFactory::size() const noexcept {
    return m_prototypes.size();
}

// ============================================================================
// XML Input (using xml_framework)
// ============================================================================

void FunctionFactory::readXml(const std::string& xmlFile) {
    using namespace xml_framework;
    
    // Load and parse the XML file
    XmlDomBuilder builder;
    auto loadResult = builder.tryLoadFile(xmlFile);
    
    if (loadResult.isError()) {
        std::ostringstream errorMessage;
        errorMessage << "FunctionFactory::readXml:\nInput xml file '"
                     << xmlFile << "' not parsed successfully.\n"
                     << "Error: " << loadResult.error().message;
        throw Exception(errorMessage.str());
    }
    
    // Get the root element
    auto rootOpt = builder.root();
    if (!rootOpt) {
        throw Exception(std::string("FunctionFactory::readXml:\n")
                        + "Empty or invalid XML document: " + xmlFile);
    }
    
    auto root = *rootOpt;
    rapidxml::xml_node<>* functionLibrary = root.node();
    
    // Verify root element is "function_library"
    std::string_view rootName(functionLibrary->name(), functionLibrary->name_size());
    if (rootName != "function_library") {
        throw Exception(std::string("FunctionFactory::readXml:\n")
                        + "Expected root element 'function_library', found '"
                        + std::string(rootName) + "' in " + xmlFile);
    }
    
    // Loop through function child elements, and add each as a Function
    // object to the prototype factory.
    for (auto* funcNode = functionLibrary->first_node("function");
         funcNode != nullptr;
         funcNode = funcNode->next_sibling("function")) {
        
        // Get the type of this function, which should be an existing 
        // (generic) Function in the factory.
        auto* typeAttr = funcNode->first_attribute("type");
        if (!typeAttr) {
            throw Exception("FunctionFactory::readXml:\n"
                            "Function element missing 'type' attribute");
        }
        std::string type(typeAttr->value(), typeAttr->value_size());
        
        // Create the function object from the base type
        Function* funcObj = nullptr;
        try {
            funcObj = create(type);
        } catch (Exception& eObj) {
            std::cerr << "FunctionFactory::readXml: "
                      << "Failed to create Function object "
                      << type << std::endl;
            throw;
        }
        
        // Set the name of this function prototype.
        auto* nameAttr = funcNode->first_attribute("name");
        if (!nameAttr) {
            delete funcObj;
            throw Exception("FunctionFactory::readXml:\n"
                            "Function element missing 'name' attribute");
        }
        std::string name(nameAttr->value(), nameAttr->value_size());
        
        // Use the type attribute as the name for use by writeXml as the type
        // information.
        funcObj->setName(type);
        
        // Fetch the parameter elements and set the Parameter data members.
        for (auto* paramNode = funcNode->first_node("parameter");
             paramNode != nullptr;
             paramNode = paramNode->next_sibling("parameter")) {
            
            // Get parameter name
            auto* paramNameAttr = paramNode->first_attribute("name");
            if (!paramNameAttr) {
                delete funcObj;
                throw Exception("FunctionFactory::readXml:\n"
                                "Parameter element missing 'name' attribute");
            }
            std::string paramName(paramNameAttr->value(), paramNameAttr->value_size());
            
            // Use the Parameter's extractDomData method to read all parameter
            // attributes from the XML node (value, min, max, free, scale, etc.)
            funcObj->parameter(paramName).extractDomData(paramNode);
        }
        
        // Add the customized function to the factory
        addFunc(name, funcObj, false);
    }
}

// ============================================================================
// XML Output (using xml_framework)
// ============================================================================

// ============================================================================
// XML Output (using xml_framework)
// ============================================================================

void FunctionFactory::writeXml(const std::string& xmlFile) const {
    using namespace xml_framework;
    
    // Create a new XML document
    XmlDomBuilder builder;
    builder.createDocument();
    
    // Create root element
    auto root = builder.createRoot("function_library");
    root.addAttribute("title", "prototype Functions");
    
    rapidxml::xml_document<>& doc = builder.document();
    rapidxml::xml_node<>* funcLib = root.node();
    
    // Loop over the Function prototypes, keeping only the derived prototypes.
    for (const auto& [name, func] : m_prototypes) {
        std::string type = func->getName();
        if (type.empty()) {
            // Skip this Function since a lack of type implies a base prototype.
            continue;
        }
        
        // Create function element
        char* allocFuncName = doc.allocate_string("function");
        auto* funcElt = doc.allocate_node(rapidxml::node_element, allocFuncName);
        
        // Add name attribute
        char* allocNameAttr = doc.allocate_string("name");
        char* allocNameVal = doc.allocate_string(name.c_str());
        funcElt->append_attribute(doc.allocate_attribute(allocNameAttr, allocNameVal));
        
        // Use the generic name of the Function object as the type attribute.
        char* allocTypeAttr = doc.allocate_string("type");
        char* allocTypeVal = doc.allocate_string(func->genericName().c_str());
        funcElt->append_attribute(doc.allocate_attribute(allocTypeAttr, allocTypeVal));
        
        // Use Function's appendParamDomElements to write parameter elements
        func->appendParamDomElements(&doc, funcElt);
        
        funcLib->append_node(funcElt);
    }
    
    // Write the XML file
    std::ofstream outFile(xmlFile.c_str());
    if (!outFile.is_open()) {
        throw Exception("FunctionFactory::writeXml:\n"
                        "Cannot open output file: " + xmlFile);
    }
    
    // Write XML declaration and DOCTYPE manually (matching original format)
    outFile << "<?xml version='1.0' standalone='no'?>\n"
            << "<!DOCTYPE function_library SYSTEM "
            << "\"$(OPTIMIZERSXMLPATH)/FunctionModels.dtd\" >\n";
    
    // Use XmlPrinter::toString to get the XML content, then write to file
    XmlPrintOptions options;
    options.omitDeclaration = true;
    options.indentString = "";
    options.newlineChar = '\n';
    
    // Print the root element (function_library) with pretty formatting
    std::string xmlContent = XmlPrinter::toString(funcLib, true, "", options);
    outFile << xmlContent;
}
} // namespace optimizers
