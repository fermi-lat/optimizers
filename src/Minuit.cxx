/** 
 * @file Minuit.cxx
 * @brief Minuit class implementation
 * @author P. Nolan
 *
 * $Header$
 */

#include <sstream>
#include "optimizers/dArg.h"
#include "optimizers/Minuit.h"
#include "optimizers/Parameter.h"
#include "optimizers/Exception.h"
#include "optimizers/OutOfBounds.h"

namespace optimizers {


  Minuit::Minuit(Statistic& stat) : Optimizer(stat), m_maxEval(200) {
    const int i5=5, i6=6, i7=7;
    mninit_(&i5, &i6, &i7);
  }

  void Minuit::setMaxEval(int n) {
    m_maxEval = n;
  }

  int Minuit::getQuality(void) const {
    return m_quality;
  }

  double Minuit::getDistance(void) const {
    return m_distance;
  }

  const std::vector<double> & Minuit::getUncertainty(void) {
    return m_uncertainty;
  }

  void Minuit::find_min(int verbose, double tol, int tolType) {

    typedef std::vector<Parameter>::iterator pptr;

    std::vector<Parameter> params;
    m_stat->getFreeParams(params);
    int errorFlag;

    int minuitVerbose = verbose - 1;
    if (minuitVerbose >= 0) {
      const int i5=5, i6=6, i7=7;
      mintio_(&i5, &i6, &i7);
    }
    std::ostringstream pline;
    pline << "SET PRINT " << minuitVerbose << std::endl;
    doCmd(pline.str()); // Set verbosity of Minuit
    doCmd("SET NOWARN");

    // Tell Minuit about parameter values, names, bounds, etc.
    for (pptr p = params.begin(); p != params.end(); p++) {
      double scale = 1.0; // Is this the best choice?
      double value = p->getValue();
      double lowerBound = p->getBounds().first;
      double upperBound = p->getBounds().second;
      int j = p - params.begin() + 1;
      mnparm_(&j, p->getName().c_str(), &value, &scale, 
	      &lowerBound, &upperBound, &errorFlag, p->getName().size());
    }

    doCmd("SET ERR 0.5");  // Delta value = 1/2: correct for likelihood
    doCmd("SET GRAD 1");  // Use gradient calculated by fcn
    
    double tolerance = 0.;
    if (tolType == ABSOLUTE) {
      tolerance = 2000. * tol;
    } else if (tolType == RELATIVE) {
      tolerance = 2000. * tol * fabs(m_stat->value());
    }
    std::ostringstream mline;
    mline << "MIGRAD " << m_maxEval << " " << tolerance << std::endl;
    int retCode = doCmd(mline.str());  // Minimize fcn
    if (retCode == 4) {
      // Abnormal termination
      throw Exception
	("Minuit abnormal termination. (No convergence?)");
    }
    else if (retCode > 0) {
      // Faulty command line
      throw Exception("Minuit bad command line");
    }
    // Normal termination.

    // Improve the quality of the Hessian matrix.
    doCmd("HESSE");

    // Extract fitted parameters
    if (verbose != 0) {
      std::cout << "Final values: " << std::endl;
    }
    for (pptr p = params.begin(); p != params.end(); p++) {
      std::vector<char> pname(10);
      double pval, error, bnd1, bnd2;
      int ivarbl;
      int j = p - params.begin() + 1;
      mnpout_(&j, &pname[0], &pval, &error, &bnd1, &bnd2, &ivarbl, 
	      pname.size());
      p->setValue(pval);
      if (verbose != 0) {
	std::cout << "  " << std::string(pname.begin(), pname.end()) 
		  << " = " << pval << std::endl;
      }
    }

    // Put new parameter values back into the Statistic
    std::vector<double> paramValues;
    for (unsigned int i = 0; i < params.size(); i++) {
      paramValues.push_back(params[i].getValue());
    }
    m_stat->setFreeParamValues(paramValues);

    // Get information about quality of minimization
    int nVariable, nparx, minStat;
    double fmin, vertDist, errDef;
    mnstat_(&fmin, &vertDist, &errDef, &nVariable, &nparx, &minStat);
    m_quality = minStat;
    m_distance = vertDist;
    if (verbose != 0) {
      std::cout << "Minuit fit quality: " << minStat << 
	"   estimated distance: " << vertDist << std::endl;
    }

    // Get parameter uncertainties
    if (verbose != 0) {
      std::cout << "Minuit parameter uncertainties:" << std::endl;
    }
    m_uncertainty.clear();
    for (int i = 1; i <= nVariable; i++) {
      double ePlus, eMinus, eParab, globCC;
      mnerrs_(&i, &ePlus, &eMinus, &eParab, &globCC);
      m_uncertainty.push_back(eParab);  // Not using MINOS, so this is it.
      if (verbose != 0) {
	std::cout << "  " << i << "  " << eParab << std::endl;
      }
    }

  } // End of find_min

  int Minuit::doCmd(std::string command) {
    // Pass a command string to Minuit
    int errorFlag = 0;
    void * statistic = static_cast<void *>(m_stat);
    mncomd_(&fcn, command.c_str(), &errorFlag, statistic,
	    command.length());
    return errorFlag;
  }

  void fcn(int* npar, double* grad, double* fcnval,
	   double* xval, int* iflag, void* futil) {
    // This is the function that Minuit minimizes
    std::vector<double> parameters(xval, xval+*npar);

    // What a hack!  Minuit thinks futil is a function 
    // pointer.  It's been hijacked to be a pointer to
    // m_stat, so this non-member function can use it.

    Statistic * statp = static_cast<Statistic *>(futil);
    try {statp->setFreeParamValues(parameters);}
    catch (OutOfBounds& e) {
      std::cerr << e.what() << std::endl;
      std::cerr << "Value " << e.value() << " is not between " 
		<< e.minValue() << " and " << e.maxValue() << std::endl;
      exit(e.code());
    }
    catch (Exception& e) {
      std::cerr << e.what() << std::endl;
      exit(e.code());
    }

    *fcnval = -statp->value();
    if (*iflag == 2) { // Return gradient values
      std::vector<double> gradient;
      statp->getFreeDerivs(gradient);
      for (int i=0; i < *npar; i++) {
	grad[i] = -gradient[i];
      }
    }
  }    
} // namespace optimizers


