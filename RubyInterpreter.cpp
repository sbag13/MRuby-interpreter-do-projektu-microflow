#include "RubyInterpreter.hpp"
#include "Exceptions.hpp"

#include <mruby/string.h>
#include <mruby/numeric.h>
#include <mruby/array.h>

using namespace std ;

namespace microflow
{
    template<typename Type>
    Type convertTo (mrb_value rubyVariable) ;

    template<>      // converts only floats and integers
    double convertTo<double> (mrb_value rubyVariable)
    {
        if (mrb_float_p (rubyVariable) || mrb_fixnum_p (rubyVariable)) {
            return mrb_float (rubyVariable) ;
        }
        else 
        {
            THROW ("Ruby exception: ruby variable is not a float or integer type") ;
        }
    }

    template<>
    std::string convertTo<string> (mrb_value rubyVariable)
    {
        if (mrb_string_p (rubyVariable)) {
            return RSTRING_PTR (rubyVariable) ;
        }
        else
        {
            THROW ("Ruby exception: ruby variable is not a string type") ;
        }
    }

    template<>  // mruby does not support integers bigger than INT_MAX 
    unsigned convertTo<unsigned> (mrb_value rubyVariable) 
    {
        if (mrb_fixnum_p (rubyVariable))
        {
            return (mrb_fixnum (rubyVariable)) ;
        }
        else
        {
            THROW ("Ruby exception: ruby variable is not a integer type, or is bigger than INT_MAX") ;
        }
    }

    template<>  // recommended in mruby instead of convertTo<unsigned>
    int convertTo<int> (mrb_value rubyVariable)
    {
        if (mrb_fixnum_p (rubyVariable))
        {
            return  (mrb_fixnum (rubyVariable)) ;
        }
        else
        {
            THROW ("Ruby exception: ruby variable is not a int type") ;
        }
    }

    template<>
    bool convertTo<bool> (mrb_value rubyVariable)
    {
        return mrb_bool (rubyVariable) ;
    }

    std::unique_ptr<MRubyInterpreter> MRubyInterpreter::
    getMRubyInterpreter ()
    {
        return std::unique_ptr<MRubyInterpreter>(new MRubyInterpreter());
    }

    MRubyInterpreter::
    MRubyInterpreter ()
    {
        initializeMRubyInterpreter () ;
    }

    MRubyInterpreter::
    ~MRubyInterpreter ()
    {          
        closeMRubyInterpreter () ;
    }

    void MRubyInterpreter::
    closeMRubyInterpreter()
    {
        mrbc_context_free (state_, context_) ;
        mrb_close (state_) ;
        state_ = nullptr ;   
    }

    mrb_value MRubyInterpreter::
    runScript (const string& code)
    {
        struct mrb_parser_state * parser_ ;
        struct RProc * proc_ ;
        
        parser_ = mrb_parse_string (state_, code.c_str(), context_) ;
        proc_ = mrb_generate_code (state_, parser_) ;
        mrb_pool_close (parser_->pool) ;
        value_ = mrb_run (state_, proc_, mrb_top_self(state_)) ;

        if (state_->exc) {
            logger << "ERROR in Ruby\n" ;
            mrb_value lasterr = mrb_obj_value (state_->exc) ;

            //class
            mrb_value klass = mrb_class_path (state_, mrb_obj_class(state_, lasterr)) ;
            logger << "class = " << convertTo<string> (klass) << endl ;

            //message
            mrb_value message = mrb_obj_as_string (state_, lasterr) ;
            logger << "message = " << convertTo<string> (message) << endl ;

            THROW ("Ruby exception") ;
        }
    }

    template<typename VariableType >
    VariableType MRubyInterpreter::
    getMRubyVariable( const std::string & variableName)
    {
        mrb_value mrb_string = mrb_str_new_cstr (state_, variableName.c_str()) ;
        mrb_sym symbol = mrb_intern_str (state_, mrb_string) ;
        mrb_value rubyVariable = mrb_gv_get (state_, symbol) ;
        VariableType variable = convertTo<VariableType>( rubyVariable ) ;

        //checking whether the variable exists
        if (mrb_equal (state_, rubyVariable, mrb_nil_value ()))    
        {
            THROW ("Ruby exception: ruby variable does not exist") ;
        }
        return variable ;
    }
    
    template double MRubyInterpreter::
    getMRubyVariable<double> (const std::string & variableName) ;

    template bool MRubyInterpreter::
    getMRubyVariable<bool> (const std::string & variableName) ;

    template std::string MRubyInterpreter::
    getMRubyVariable<std::string> (const std::string & variableName) ;

    template unsigned MRubyInterpreter::
    getMRubyVariable<unsigned> (const std::string & variableName) ;

    template int MRubyInterpreter::
    getMRubyVariable<int> (const std::string & variableName) ;

    // Used in methods called by Ruby interpreter - these methods must be static.
    static NodeLayout * nodeLayoutPtr = nullptr ;
    static ModificationRhoU * modificationsRhoUPtr = nullptr ;

    static mrb_value setNodeBaseType (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 4) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_nodeZ ;     
        mrb_int mrb_nodeX ;
        mrb_int mrb_nodeY ;
        mrb_value mrb_nodeBaseTypeName ;

        mrb_get_args (state, "iiiS", &mrb_nodeX, &mrb_nodeY, &mrb_nodeZ, &mrb_nodeBaseTypeName) ;

        unsigned nodeX = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeX)) ;
        unsigned nodeY = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeY)) ;
        unsigned nodeZ = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeZ)) ;
        string nodeBaseTypeName = convertTo<string> (mrb_nodeBaseTypeName) ;

        auto node = nodeLayoutPtr->getNodeType (nodeX,nodeY,nodeZ) ;
	    node.setBaseType (fromString<NodeBaseType> (nodeBaseTypeName)) ;
	    nodeLayoutPtr->setNodeType (nodeX, nodeY, nodeZ, node) ;
        return mrb_nil_value () ;
    }

    static mrb_value setNodePlacementModifier (mrb_state * state, mrb_value self)
    {
        if ((mrb_get_argc (state)) != 4) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_nodeX ;         
        mrb_int mrb_nodeY ;
        mrb_int mrb_nodeZ ;
        mrb_value mrb_placementModifierName ;

        mrb_get_args (state, "iiiS", &mrb_nodeX, &mrb_nodeY, &mrb_nodeZ, &mrb_placementModifierName) ;

        unsigned nodeX = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeX)) ;
        unsigned nodeY = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeY)) ;
        unsigned nodeZ = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeZ)) ;
        string placementModifierName = convertTo<string> (mrb_placementModifierName) ;

        auto node = nodeLayoutPtr->getNodeType (nodeX,nodeY,nodeZ) ;
	    node.setPlacementModifier (fromString<PlacementModifier> (placementModifierName)) ;
	    nodeLayoutPtr->setNodeType (nodeX,nodeY,nodeZ, node) ;

	    return mrb_nil_value () ;
    }

    static mrb_value setNodeRhoPhysical (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 4) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_nodeX ;           
        mrb_int mrb_nodeY ;
        mrb_int mrb_nodeZ ;
        mrb_float mrb_rhoPhysical;

        mrb_get_args (state, "iiif", &mrb_nodeX, &mrb_nodeY, &mrb_nodeZ, &mrb_rhoPhysical) ;

        unsigned nodeX = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeX)) ;
        unsigned nodeY = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeY)) ;
        unsigned nodeZ = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeZ)) ;
        double rhoPhysical = convertTo<double> (mrb_float_value (state, mrb_rhoPhysical)) ;
        
        modificationsRhoUPtr->addRhoPhysical (Coordinates (nodeX, nodeY, nodeZ), rhoPhysical) ;

        return mrb_nil_value () ;
    }

    static mrb_value setNodeRhoBoundaryPhysical (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 4) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_nodeX ;       
        mrb_int mrb_nodeY ;
        mrb_int mrb_nodeZ ;
        mrb_float mrb_rhoPhysical;

        mrb_get_args (state, "iiif", &mrb_nodeX, &mrb_nodeY, &mrb_nodeZ, &mrb_rhoPhysical) ;


        unsigned nodeX = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeX)) ;
        unsigned nodeY = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeY)) ;
        unsigned nodeZ = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeZ)) ;
        double rhoPhysical = convertTo<double> (mrb_float_value (state, mrb_rhoPhysical)) ;

        modificationsRhoUPtr->addRhoBoundaryPhysical (Coordinates (nodeX, nodeY, nodeZ), rhoPhysical) ;

        return mrb_nil_value () ;
    }

    static mrb_value setNodeUPhysical (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 4) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_nodeX ;          
        mrb_int mrb_nodeY ;
        mrb_int mrb_nodeZ ;
        mrb_value mrb_uPhysical;

        mrb_get_args (state, "iiiA", &mrb_nodeX, &mrb_nodeY, &mrb_nodeZ, &mrb_uPhysical) ;

        unsigned nodeX = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeX)) ;
        unsigned nodeY = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeY)) ;
        unsigned nodeZ = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeZ)) ;
        double ux = convertTo<double> (mrb_ary_entry (mrb_uPhysical, 0)) ;
	    double uy = convertTo<double> (mrb_ary_entry (mrb_uPhysical, 1)) ;
	    double uz = convertTo<double> (mrb_ary_entry (mrb_uPhysical, 2)) ;

        modificationsRhoUPtr->addUPhysical (Coordinates (nodeX, nodeY, nodeZ), ux,uy,uz) ;

        return mrb_nil_value () ;
    }

    static mrb_value setNodeUBoundaryPhysical (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 4) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_nodeX ;          
        mrb_int mrb_nodeY ;
        mrb_int mrb_nodeZ ;
        mrb_value mrb_uPhysical;

        mrb_get_args (state, "iiiA", &mrb_nodeX, &mrb_nodeY, &mrb_nodeZ, &mrb_uPhysical) ;

        unsigned nodeX = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeX)) ;
        unsigned nodeY = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeY)) ;
        unsigned nodeZ = convertTo<unsigned> (mrb_fixnum_value (mrb_nodeZ)) ;
        double ux = convertTo<double> (mrb_ary_entry (mrb_uPhysical, 0)) ;
	    double uy = convertTo<double> (mrb_ary_entry (mrb_uPhysical, 1)) ;
	    double uz = convertTo<double> (mrb_ary_entry (mrb_uPhysical, 2)) ;

        modificationsRhoUPtr->addUBoundaryPhysical (Coordinates (nodeX, nodeY, nodeZ), ux,uy,uz) ;

        return mrb_nil_value () ;
    }

    mrb_value createMRubyObject (mrb_state* mrb, const std::string& className)
    {
        struct RClass *mrb_class ;
        mrb_value mrb_object ;
        mrb_class = mrb_define_class(mrb, className.c_str (), mrb->object_class) ;
        mrb_object = mrb_obj_new (mrb, mrb_class, 0, NULL) ;

        return mrb_object ;
    }

    static mrb_value getNode (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 3) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_int mrb_X ;          
        mrb_int mrb_Y ;
        mrb_int mrb_Z ;

        mrb_get_args (state, "iii", &mrb_X, &mrb_Y, &mrb_Z) ;

        unsigned x = convertTo<unsigned> (mrb_fixnum_value (mrb_X)) ;
        unsigned y = convertTo<unsigned> (mrb_fixnum_value (mrb_Y)) ;
        unsigned z = convertTo<unsigned> (mrb_fixnum_value (mrb_Z)) ;

        mrb_value node = createMRubyObject (state, "Node") ;

        NodeType nodeType ;
        Coordinates coordinates (x, y, z) ;

        Size size = nodeLayoutPtr->getSize () ;
        if (size.areCoordinatesInLimits (coordinates))
        {
            nodeType = nodeLayoutPtr->getNodeType (coordinates) ;
        }
        else
        {
            logger << "WARNING: Can not get node type at " << coordinates 
					 << ", coordinates outside of " << size << "\n" ;
		    return mrb_nil_value () ;
        }

        std::string baseTypeName = toString (nodeType.getBaseType()) ;
        std::string placementModifierName = toString (nodeType.getPlacementModifier()) ;

        mrb_iv_set(state, node, 
                    mrb_intern_str (state, mrb_str_new_cstr (state, "@baseType")),
                    mrb_str_new_cstr (state, baseTypeName.c_str())) ;
        mrb_iv_set(state, node, 
                    mrb_intern_str (state, mrb_str_new_cstr (state, "@placementModifier")),
                    mrb_str_new_cstr (state, placementModifierName.c_str())) ;

        return node ;
    }

    static mrb_value getSize (mrb_state * state, mrb_value self) 
    {
        if ((mrb_get_argc (state)) != 0) 
        {
            std::string comunicate {"Wrong number of arguments in :"} ; 
            comunicate.append (__func__) ;
            THROW (comunicate) ;
        }

        mrb_value size = createMRubyObject (state, "Size") ;

        Size nodeLayoutSize = nodeLayoutPtr->getSize () ;

        mrb_iv_set (state, size, 
                    mrb_intern_str (state, mrb_str_new_cstr (state, "@width")), 
                    mrb_fixnum_value (nodeLayoutSize.getWidth ())) ;
        mrb_iv_set (state, size, 
                    mrb_intern_str (state, mrb_str_new_cstr (state, "@height")),
                    mrb_fixnum_value (nodeLayoutSize.getHeight ())) ;
        mrb_iv_set (state, size, 
                    mrb_intern_str (state, mrb_str_new_cstr (state, "@depth")),
                    mrb_fixnum_value (nodeLayoutSize.getDepth ())) ;

        return size ;
    }

    static void
    initializeRubyModifyLayout(mrb_state * state)
    {
        mrb_define_method (state, state->kernel_module, 
                    "setNodeBaseType", setNodeBaseType, MRB_ARGS_REQ (4)) ;
        mrb_define_method (state, state->kernel_module, 
                    "setNodePlacementModifier", setNodePlacementModifier, MRB_ARGS_REQ (4)) ;
        mrb_define_method (state, state->kernel_module, 
                    "setNodeRhoPhysical", setNodeRhoPhysical, MRB_ARGS_REQ (4)) ;
        mrb_define_method (state, state->kernel_module, 
                    "setNodeRhoBoundaryPhysical", setNodeRhoBoundaryPhysical, MRB_ARGS_REQ (4)) ;
        mrb_define_method (state, state->kernel_module, 
                    "setNodeUPhysical", setNodeUPhysical, MRB_ARGS_REQ (4)) ;
        mrb_define_method (state, state->kernel_module, 
                    "setNodeUBoundaryPhysical", setNodeUBoundaryPhysical, MRB_ARGS_REQ (4)) ;
        mrb_define_method (state, state->kernel_module, 
                    "getNode", getNode, MRB_ARGS_REQ (3)) ;
        mrb_define_method (state, state->kernel_module, 
                    "getSize", getSize, MRB_ARGS_NONE ()) ;
    }

    ModificationRhoU MRubyInterpreter::
    modifyNodeLayout (NodeLayout & nodeLayout, const std::string & rubyCode)
    {
        initializeRubyModifyLayout (state_) ;

        	nodeLayoutPtr = &nodeLayout ;
	        ModificationRhoU modifications ;
	        modificationsRhoUPtr = &modifications ;

            //TODO: Use xxd.
            #define STRINGIFY(x) #x
            const char * script = //TODO: should I move it to RubyScripts.hpp ?
                    #include "modifyNodeLayout.rb"
                    ;
            #undef STRINGIFY

            std::string code = script + rubyCode ;

            runScript (code.c_str ()) ;

            nodeLayoutPtr = nullptr ;
            modificationsRhoUPtr = nullptr ;

            return modifications ;
    }

    void MRubyInterpreter::
    initializeMRubyInterpreter()
    {
        state_ = mrb_open () ;
        context_ = mrbc_context_new (state_) ;
        if (!state_) 
        {
            THROW ("Ruby exception: could not open ruby interpreter") ;
        }
    }
}

