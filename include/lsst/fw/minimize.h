// -*- LSST-C++ -*-
#ifndef LSST_FW_minimize_H
#define LSST_FW_minimize_H
/**
 * \file
 *
 * Class that Minuit knows how to minimize, that contains an lsst::fw::function::Function
 *
 * \author Andrew Becker and Russell Owen
 *
 * \ingroup fw
 */

#include <Minuit/FCNBase.h>
#include <lsst/mwi/data/LsstData.h>
#include <lsst/fw/Function.h>
#include <boost/shared_ptr.hpp>

namespace lsst {
namespace fw {
namespace function {

    /**
     * \brief Results from minimizing a function
     */ 
    struct FitResults {
    public:
        bool isValid;   ///< true if the fit converged; false otherwise
        double chiSq;   ///< chi squared; may be nan or infinite, but only if isValid false
        std::vector<double> parameterList; ///< fit parameters
        std::vector<std::pair<double,double> > parameterErrorList; ///< negative,positive (1 sigma?) error for each parameter
    };

    /**
     * \brief Minuit wrapper for a function(x)
     */
    template<typename ReturnT>
    class MinimizerFunctionBase1 : public FCNBase, private lsst::mwi::data::LsstBase {
    public:
        explicit MinimizerFunctionBase1();
        explicit MinimizerFunctionBase1(
            boost::shared_ptr<lsst::fw::function::Function1<ReturnT> > theFunctionPtr,
            std::vector<double> const &measurementList,
            std::vector<double> const &varianceList,
            std::vector<double> const &xPositionList,
            double errorDef
        );
        virtual ~MinimizerFunctionBase1() {};
        // Required by FCNBase
        virtual double up() const {return _errorDef;}
        virtual double operator() (const std::vector<double>&) const;

        //void minimizee(std::vector<double> &parameters,
        //std::vector<std::pair<double,double> > &errors);
        inline std::vector<double> getMeasurements() const {return _measurementList;}
        inline std::vector<double> getVariances() const {return _varianceList;}
        inline std::vector<double> getPositions() const {return _xPositionList;}
        inline void setErrorDef(double def) {_errorDef=def;}
    private:
        boost::shared_ptr<lsst::fw::function::Function1<ReturnT> > _theFunctionPtr;
        std::vector<double> _measurementList;
        std::vector<double> _varianceList;
        std::vector<double> _xPositionList;
        double _errorDef;
    };
        
    /**
     * \brief Minuit wrapper for a function(x, y)
     */
    template<typename ReturnT>
    class MinimizerFunctionBase2 : public FCNBase, private lsst::mwi::data::LsstBase {
    public:
        explicit MinimizerFunctionBase2();
        explicit MinimizerFunctionBase2(
            boost::shared_ptr<lsst::fw::function::Function2<ReturnT> > theFunctionPtr,
            std::vector<double> const &measurementList,
            std::vector<double> const &varianceList,
            std::vector<double> const &xPositionList,
            std::vector<double> const &yPositionList,
            double errorDef
        );
        virtual ~MinimizerFunctionBase2() {};
        // Required by FCNBase
        virtual double up() const {return _errorDef;}
        virtual double operator() (const std::vector<double>&) const;
        
        //void minimizee(std::vector<double> &parameters,
        //std::vector<std::pair<double,double> > &errors);
        inline std::vector<double> getMeasurements() const {return _measurementList;}
        inline std::vector<double> getVariances() const {return _varianceList;}
        inline std::vector<double> getPosition1() const {return _xPositionList;}
        inline std::vector<double> getPosition2() const {return _yPositionList;}
        inline void setErrorDef(double def) {_errorDef=def;}
    private:
        boost::shared_ptr<lsst::fw::function::Function2<ReturnT> > _theFunctionPtr;
        std::vector<double> _measurementList;
        std::vector<double> _varianceList;
        std::vector<double> _xPositionList;
        std::vector<double> _yPositionList;
        double _errorDef;
    };
        
    template<typename ReturnT>
    FitResults minimize(
        boost::shared_ptr<lsst::fw::function::Function1<ReturnT> > functionPtr,
        std::vector<double> const &initialParameterList,
        std::vector<double> const &stepSizeList,
        std::vector<double> const &measurementList,
        std::vector<double> const &varianceList,
        std::vector<double> const &xPositionList,
        double errorDef
    );

    template<typename ReturnT>
    FitResults minimize(
        boost::shared_ptr<lsst::fw::function::Function2<ReturnT> > functionPtr,
        std::vector<double> const &initialParameterList,
        std::vector<double> const &stepSizeList,
        std::vector<double> const &measurementList,
        std::vector<double> const &varianceList,
        std::vector<double> const &xPositionList,
        std::vector<double> const &yPositionList,
        double errorDef
    );
    
}   // namespace function        
}   // namespace fw
}   // namespace lsst

#ifndef SWIG // don't bother SWIG with .cc files
#include <lsst/fw/minimize.cc>
#endif

#endif // !defined(LSST_FW_minimize_H)
