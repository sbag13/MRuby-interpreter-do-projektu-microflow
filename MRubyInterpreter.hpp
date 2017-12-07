/*
Created by Szymon Bagiński
baginski.szymon@gmail.com
Wrocław University of Technology
Dec 2017
*/

#ifndef MRUBY_INTERPRETER_HPP
#define MRUBY_INTERPRETER_HPP

#include <memory>
#include <string>
#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/proc.h>
#include <mruby/variable.h>

#include "Coordinates.hpp"
#include "NodeLayout.hpp"
#include "ModificationRhoU.hpp"

namespace microflow
{
    class MRubyInterpreter
    {       
    public:

        static std::unique_ptr<MRubyInterpreter> getMRubyInterpreter () ;
        ~MRubyInterpreter () ;
        mrb_value runScript (const std::string&) ;

        template<class VariableType >
        VariableType getMRubyVariable (const std::string & variableName) ;
        
        ModificationRhoU modifyNodeLayout (NodeLayout & nodeLayout, const std::string & rubyCode) ;

    private:
        MRubyInterpreter () ; 
        void initializeMRubyInterpreter () ;
        void closeMRubyInterpreter ();

        mrb_state* state_ = nullptr ;
        mrbc_context * context_ ;
        mrb_value value_ ;        
    } ;
}

#endif