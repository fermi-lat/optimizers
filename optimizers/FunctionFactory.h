/**
 * @file FunctionFactory.h
 * @brief Prototype pattern for generating Function objects.
 * @author J. Chiang
 *
 * $Header$
 */

#ifndef optimizers_FunctionFactory_h
#define optimizers_FunctionFactory_h

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <string_view>

#include "optimizers/Function.h"
#include "optimizers/Exception.h"

namespace optimizers {

/**
 * @class FunctionFactory
 *
 * @brief Use the prototype pattern to supply customized Function
 * objects, the parameters of which are specified by an xml input
 * file.
 *
 * This implementation uses the RapidXML-based xml_framework for all
 * XML parsing and generation, eliminating dependency on Xerces-C.
 */
class FunctionFactory {

public:
    FunctionFactory();

    virtual ~FunctionFactory();

    // Non-copyable
    FunctionFactory(const FunctionFactory&) = delete;
    FunctionFactory& operator=(const FunctionFactory&) = delete;

    // Movable
    FunctionFactory(FunctionFactory&&) noexcept = default;
    FunctionFactory& operator=(FunctionFactory&&) noexcept = default;

    /**
     * @brief Create a Function object by name
     * @param name The name of the function prototype
     * @return Pointer to a cloned Function object (caller takes ownership)
     * @throws Exception if the function name is not found
     */
    [[nodiscard]] Function* create(const std::string& name);

    /**
     * @brief Add a function prototype with explicit name
     * @param name The name to register the function under
     * @param func Pointer to the function prototype
     * @param fromClone If true, stores a clone; if false, takes ownership directly
     * @throws std::runtime_error if a function with this name already exists
     */
    void addFunc(const std::string& name, Function* func, bool fromClone = true);

    /**
     * @brief Add a function prototype using its generic name
     * @param func Pointer to the function prototype
     * @param fromClone If true, stores a clone; if false, takes ownership directly
     */
    void addFunc(Function* func, bool fromClone = true);

    /**
     * @brief Get list of all registered function names
     * @param funcNames Output vector to receive function names
     */
    void getFunctionNames(std::vector<std::string>& funcNames) const;

    /**
     * @brief Get list of all registered function names (C++17 style)
     * @return Vector of registered function names
     */
    [[nodiscard]] std::vector<std::string> getFunctionNames() const;

    /**
     * @brief Load function prototypes from XML file
     * @param xmlFile Path to the XML file
     * @throws Exception on parse error or invalid format
     */
    void readXml(const std::string& xmlFile);

    /**
     * @brief Write function prototypes to XML file
     * @param outputFile Path to the output XML file
     * @throws Exception on write error
     */
    void writeXml(const std::string& outputFile) const;

    /**
     * @brief Check if a function prototype exists
     * @param name The function name to check
     * @return true if the function exists
     */
    [[nodiscard]] bool hasFunction(const std::string& name) const noexcept;

    /**
     * @brief Get the number of registered prototypes
     * @return Number of function prototypes
     */
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::map<std::string, Function*> m_prototypes;
};

} // namespace optimizers

#endif // optimizers_FunctionFactory_h
