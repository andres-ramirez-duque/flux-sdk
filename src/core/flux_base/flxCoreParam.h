
/*
 *---------------------------------------------------------------------------------
 *
 * Copyright (c) 2022-2024, SparkFun Electronics Inc.
 *
 * SPDX-License-Identifier: MIT
 *
 *---------------------------------------------------------------------------------
 */

// flxCoreParam.h
//
// Defines the objects that make up the "parameter" system of the framework

#pragma once

#include <string>
#include <vector>

#include "flxCoreInterface.h"
#include "flxCoreProps.h"
#include "flxCoreTypes.h"
#include "flxUtils.h"

// Define a type used to enumerate parameter value types (not data types, but values, like temp, accel)

typedef uint16_t flxParamValueType_t;

const flxParamValueType_t kParamValueNone = 0;

//----------------------------------------------------------------------------------------
// flxParameter
//
// Base/Core Parameter Class
//
// From an abstract sense, a basic parameter   - nothing more
//
class flxParameter : public flxDescriptor
{
    bool _isEnabled;
    flxParamValueType_t _valueType;

  public:
    flxParameter() : _isEnabled{true}, _valueType{kParamValueNone}
    {
    }

    bool enabled(void)
    {
        return _isEnabled;
    }

    flxParamValueType_t valueType(void)
    {
        return _valueType;
    }
    void setValueType(flxParamValueType_t type)
    {
        _valueType = type;
    }

    virtual void setEnabled(bool enabled)
    {
        _isEnabled = enabled;
    };
    virtual flxDataType_t type(void) = 0;
};

// We want to bin parameters as input and output for storing different
// arguments lists per object type via overloading. So define some simple classes

class flxParameterIn : public flxParameter
{
  public:
    virtual flxEditResult_t editValue(flxDataEditor &) = 0;
    virtual bool setValue(flxDataVariable &) = 0;
    virtual flxDataLimit *dataLimit(void) = 0;
    std::string to_string()
    {
        return std::string(name());
    }; // for consistancy
};

#define kParameterOutFlagArray 0x01

// class flxParameterOut : public flxParameter, public flxDataOut
class flxParameterOut : public flxParameter
{
  public:
    flxParameterOut() : _flags{0}
    {
    }
    flxParameterOut(uint8_t flags) : _flags{flags}
    {
    }

    virtual flxDataType_t type(void) = 0;
    // Some types need precision - just make it generic
    virtual uint16_t precision(void)
    {
        return 0;
    };

    // This is used with covariant returns values of the sub-classes.
    // Returns the property pointer for a given class
    virtual flxParameterOut *accessor() = 0;

    // flags -- used to highlight attributes of the output

    uint8_t flags()
    {
        return _flags;
    }

  protected:
    void setFlag(uint8_t flag)
    {
        _flags |= flag;
    }

  private:
    uint8_t _flags;
};
// simple def - list of parameters
using flxParameterInList = std::vector<flxParameterIn *>;
using flxParameterOutList = std::vector<flxParameterOut *>;

//----------------------------------------------------------------------------------------
// flxParameterContainer
//
// Define interface/class to manage a list of input and output parameters
//
//
// The intent is to add this into other classes that want to expose parameters.
//
class _flxParameterContainer
{

  public:
    //---------------------------------------------------------------------------------
    void addParameter(flxParameterIn *newParam, bool head = false)
    {
        // Insert at the head?
        if (head)
            _input_parameters.insert(_input_parameters.begin(), newParam);
        else
            _input_parameters.push_back(newParam);
    };

    //---------------------------------------------------------------------------------
    void addParameter(flxParameterIn &newParam, bool head = false)
    {
        addParameter(&newParam, head);
    };
    //---------------------------------------------------------------------------------
    void removeParameter(flxParameterIn *rmParam)
    {
        auto iter = std::find(_input_parameters.begin(), _input_parameters.end(), rmParam);

        if (iter != _input_parameters.end())
            _input_parameters.erase(iter);
    }

    //---------------------------------------------------------------------------------
    void removeParameter(flxParameterIn &rmParam)
    {
        removeParameter(&rmParam);
    }

    //---------------------------------------------------------------------------------
    size_t nInputParameters()
    {
        return _input_parameters.size();
    }

    //---------------------------------------------------------------------------------
    void addParameter(flxParameterOut *newParam, bool head = false)
    {
        // Insert at the head?
        if (head)
            _output_parameters.insert(_output_parameters.begin(), newParam);
        else
            _output_parameters.push_back(newParam);
    };

    //---------------------------------------------------------------------------------
    void addParameter(flxParameterOut &newParam, bool head = false)
    {
        addParameter(&newParam, head);
    };

    //---------------------------------------------------------------------------------
    void removeParameter(flxParameterOut *rmParam)
    {
        auto iter = std::find(_output_parameters.begin(), _output_parameters.end(), rmParam);

        if (iter != _output_parameters.end())
            _output_parameters.erase(iter);
    }

    //---------------------------------------------------------------------------------
    void removeParameter(flxParameterOut &rmParam)
    {
        removeParameter(&rmParam);
    }
    //---------------------------------------------------------------------------------
    size_t nOutputParameters()
    {
        return _output_parameters.size();
    }

    //---------------------------------------------------------------------------------
    flxParameterOutList &getOutputParameters(void)
    {
        return _output_parameters;
    };

    //---------------------------------------------------------------------------------
    flxParameterInList &getInputParameters(void)
    {
        return _input_parameters;
    };

  private:
    flxParameterInList _input_parameters;
    flxParameterOutList _output_parameters;
};

//----------------------------------------------------------------------------------------------------
// flxParameterOutScalar

class flxParameterOutScalar : public flxParameterOut, public flxDataOut
{
  public:
    // mostly a
    flxParameterOutScalar *accessor()
    {
        return this;
    }
    virtual flxDataType_t type(void) = 0;
};
//----------------------------------------------------------------------------------------------------
// flxParameterOut
//
// Output Parameter Template
//
//

template <class T, class Object, T (Object::*_getter)()>
class _flxParameterOut : public _flxDataOut<T>, public flxParameterOutScalar
{
    Object *my_object; // Pointer to the containing object

  public:
    _flxParameterOut() : my_object{nullptr}
    {
    }

    _flxParameterOut(Object *me) : my_object(me)
    {
    }
    //---------------------------------------------------------------------------------
    // return our data type
    flxDataType_t type()
    {
        return _flxDataOut<T>::type();
    };

    //---------------------------------------------------------------------------------
    // to register the parameter - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     parameter_object(this);
    //
    // This allows the parameter to add itself to the containing objects list of
    // parameter.
    //
    // Also the containing object is needed to call the getter/setter methods on that object
    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "flxParameterOut: invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }

    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }
    // override to deal with dirty status of object.
    void setEnabled(bool bEnabled)
    {
        if (bEnabled == this->enabled())
            return;

        this->flxParameter::setEnabled(bEnabled);

        if (my_object)
            my_object->setIsDirty();
    }

    //---------------------------------------------------------------------------------
    // get/set syntax
    T get() const
    {
        if (!my_object) // would normally throw an exception, but not very Arduino like!
        {
            flxLogM_E(kMsgParentObjNotSet, "output parameter");
            return (T)0;
        }
        return (my_object->*_getter)();
    }

    //---------------------------------------------------------------------------------
    // get -> parameter()
    T operator()() const
    {
        return get();
    };

    bool getBool()
    {
        return _flxDataOut<T>::getBool();
    };
    int8_t getInt8()
    {
        return _flxDataOut<T>::getInt8();
    };
    int16_t getInt16()
    {
        return _flxDataOut<T>::getInt16();
    };
    int32_t getInt32()
    {
        return _flxDataOut<T>::getInt32();
    };
    uint8_t getUInt8()
    {
        return _flxDataOut<T>::getUInt8();
    };
    uint16_t getUInt16()
    {
        return _flxDataOut<T>::getUInt16();
    };
    uint32_t getUInt32()
    {
        return _flxDataOut<T>::getUInt32();
    };
    float getFloat()
    {
        return _flxDataOut<T>::getFloat();
    };
    double getDouble()
    {
        return _flxDataOut<T>::getDouble();
    };
    std::string getString()
    {
        return _flxDataOut<T>::getString();
    };
};

// Define by type
template <class Object, bool (Object::*_getter)()> using flxParameterOutBool = _flxParameterOut<bool, Object, _getter>;

template <class Object, int8_t (Object::*_getter)()>
using flxParameterOutInt8 = _flxParameterOut<int8_t, Object, _getter>;

template <class Object, int16_t (Object::*_getter)()>
using flxParameterOutInt16 = _flxParameterOut<int16_t, Object, _getter>;

template <class Object, int32_t (Object::*_getter)()>
using flxParameterOutInt32 = _flxParameterOut<int32_t, Object, _getter>;

template <class Object, uint8_t (Object::*_getter)()>
using flxParameterOutUInt8 = _flxParameterOut<uint8_t, Object, _getter>;

template <class Object, uint16_t (Object::*_getter)()>
using flxParameterOutUInt16 = _flxParameterOut<uint16_t, Object, _getter>;

template <class Object, uint32_t (Object::*_getter)()>
using flxParameterOutUInt32 = _flxParameterOut<uint32_t, Object, _getter>;

template <class Object, float (Object::*_getter)()>
class flxParameterOutFloat : public _flxParameterOut<float, Object, _getter>
{
  public:
    flxParameterOutFloat() : _precision(3)
    {
    }
    void setPrecision(uint16_t prec)
    {
        _precision = prec;
    }
    uint16_t precision(void)
    {
        return _precision;
    }

  private:
    uint16_t _precision;
};

template <class Object, double (Object::*_getter)()>
class flxParameterOutDouble : public _flxParameterOut<double, Object, _getter>
{
  public:
    flxParameterOutDouble() : _precision(3)
    {
    }
    void setPrecision(uint16_t prec)
    {
        _precision = prec;
    }
    uint16_t precision(void)
    {
        return _precision;
    }

  private:
    uint16_t _precision;
};
//----------------------------------------------------------------------------------------------------
// flxParameterOutString
//
// Strings are special
// Output Parameter Template
//
//
template <class Object, std::string (Object::*_getter)()>
class flxParameterOutString : public flxParameterOutScalar, public _flxDataOutString
{
    Object *my_object; // Pointer to the containing object

  public:
    flxParameterOutString() : my_object{nullptr}
    {
    }

    // type
    flxDataType_t type(void)
    {
        return _flxDataOutString::type();
    };
    //---------------------------------------------------------------------------------
    // to register the parameter - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     parameter_object(this);
    //
    // This allows the parameter to add itself to the containing objects list of
    // parameter.
    //
    // Also the containing object is needed to call the getter/setter methods on that object
    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "OutString: invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }

    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }
    // override to deal with dirty status of object.
    void setEnabled(bool bEnabled)
    {
        if (bEnabled == enabled())
            return;

        this->flxParameter::setEnabled(bEnabled);

        if (my_object)
            my_object->setIsDirty();
    }

    //---------------------------------------------------------------------------------
    // get/set syntax
    std::string get() const
    {
        if (!my_object) // would normally throw an exception, but not very Arduino like!
        {
            flxLogM_E(kMsgParentObjNotSet, "output parameter");
            return std::string("");
        }

        return (my_object->*_getter)();
    }

    //---------------------------------------------------------------------------------
    // get -> parameter()
    std::string operator()() const
    {
        return get();
    };

    bool getBool()
    {
        return _flxDataOutString::getBool();
    };
    int8_t getInt8()
    {
        return _flxDataOutString::getInt8();
    };
    int16_t getInt16()
    {
        return _flxDataOutString::getInt16();
    };
    int32_t getInt32()
    {
        return _flxDataOutString::getInt32();
    };
    uint8_t getUInt8()
    {
        return _flxDataOutString::getUInt8();
    };
    uint16_t getUInt16()
    {
        return _flxDataOutString::getUInt16();
    };
    uint32_t getUInt32()
    {
        return _flxDataOutString::getUInt32();
    };
    float getFloat()
    {
        return _flxDataOutString::getFloat();
    };
    double getDouble()
    {
        return _flxDataOutString::getDouble();
    };
    std::string getString()
    {
        return _flxDataOutString::getString();
    };
};

//----------------------------------------------------------------------------------------------------
// flxParameterOutArray

class flxParameterOutArray : public flxParameterOut
{
  public:
    flxParameterOutArray() : flxParameterOut{kParameterOutFlagArray}
    {
    }

    flxParameterOutArray *accessor()
    {
        return this;
    }

    virtual flxDataArray *get(void) = 0;
};

//---------------------------------------------------------------------------------------

template <class T, class Object, bool (Object::*_getter)(flxDataArrayType<T> *)>
class flxParameterOutArrayType : public flxParameterOutArray
{

    Object *my_object; // Pointer to the containing object

  public:
    flxParameterOutArrayType() : my_object{nullptr}
    {
    }

    //---------------------------------------------------------------------------------
    // return our data type
    flxDataType_t type()
    {
        return flxGetTypeOf<T>();
    };

    //---------------------------------------------------------------------------------
    // to register the parameter - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     parameter_object(this);
    //
    // This allows the parameter to add itself to the containing objects list of
    // parameter.
    //
    // Also the containing object is needed to call the getter/setter methods on that object
    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "OutArray : invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }

    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }
    // override to deal with dirty status of object.
    void setEnabled(bool bEnabled)
    {
        if (bEnabled == enabled())
            return;

        this->flxParameter::setEnabled(bEnabled);

        if (my_object)
            my_object->setIsDirty();
    }
    //---------------------------------------------------------------------------------
    // get the value of the parameter.
    //
    // NOTE - using smart pointer/shared pointer for the return value. This will automatically
    //        free memory when the pointer goes out of scope.

    flxDataArrayType<T> *get(void)
    {
        if (!my_object) // would normally throw an exception, but not very Arduino like!
        {
            flxLogM_E(kMsgParentObjNotSet, "output parameter");
            return nullptr;
        }
        flxDataArrayType<T> *data = new flxDataArrayType<T>;
        bool bstatus = (my_object->*_getter)(data);

        if (!bstatus)
        {
            delete data;
            return nullptr;
        }

        return data;
    }

    // //---------------------------------------------------------------------------------
    // // get -> parameter()
    // bool operator()(flxDataArrayType<T> & data)
    // {
    //     return get(data);
    // };
};

// Define by type
template <class Object, bool (Object::*_getter)(flxDataArrayType<bool> *)>
using flxParameterOutArrayBool = flxParameterOutArrayType<bool, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<int8_t> *)>
using flxParameterOutArrayInt8 = flxParameterOutArrayType<int8_t, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<int16_t> *)>
using flxParameterOutArrayInt16 = flxParameterOutArrayType<int16_t, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<int32_t> *)>
using flxParameterOutArrayInt32 = flxParameterOutArrayType<int32_t, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<uint8_t> *)>
using flxParameterOutArrayUInt8 = flxParameterOutArrayType<uint8_t, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<uint16_t> *)>
using flxParameterOutArrayUInt16 = flxParameterOutArrayType<uint16_t, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<uint32_t> *)>
using flxParameterOutArrayUInt32 = flxParameterOutArrayType<uint32_t, Object, _getter>;

template <class Object, bool (Object::*_getter)(flxDataArrayType<float> *)>
class flxParameterOutArrayFloat : public flxParameterOutArrayType<float, Object, _getter>
{
  public:
    flxParameterOutArrayFloat() : _precision(3)
    {
    }
    void setPrecision(uint16_t prec)
    {
        _precision = prec;
    }
    uint16_t precision(void)
    {
        return _precision;
    }

  private:
    uint16_t _precision;
};

template <class Object, bool (Object::*_getter)(flxDataArrayType<double> *)>
class flxParameterOutArrayDouble : public flxParameterOutArrayType<double, Object, _getter>
{
  public:
    flxParameterOutArrayDouble() : _precision(3)
    {
    }
    void setPrecision(uint16_t prec)
    {
        _precision = prec;
    }
    uint16_t precision(void)
    {
        return _precision;
    }

  private:
    uint16_t _precision;
};

template <class Object, bool (Object::*_getter)(flxDataArrayString *)>
class flxParameterOutArrayString : public flxParameterOutArray
{

    Object *my_object; // Pointer to the containing object

  public:
    flxParameterOutArrayString() : my_object{nullptr}
    {
    }

    //---------------------------------------------------------------------------------
    // return our data type
    flxDataType_t type()
    {
        return flxTypeString;
    };

    //---------------------------------------------------------------------------------
    // to register the parameter - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     parameter_object(this);
    //
    // This allows the parameter to add itself to the containing objects list of
    // parameter.
    //
    // Also the containing object is needed to call the getter/setter methods on that object
    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "OutArray: Invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }
    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }
    // override to deal with dirty status of object.
    void setEnabled(bool bEnabled)
    {
        if (bEnabled == enabled())
            return;

        this->flxParameter::setEnabled(bEnabled);

        if (my_object)
            my_object->setIsDirty();
    }
    //---------------------------------------------------------------------------------
    // get the value of the parameter.
    //
    // NOTE - using smart pointer/shared pointer for the return value. This will automatically
    //        free memory when the pointer goes out of scope.

    flxDataArrayString *get(void)
    {
        if (!my_object) // would normally throw an exception, but not very Arduino like!
        {
            flxLogM_E(kMsgParentObjNotSet, "output parameter");
            return nullptr;
        }
        flxDataArrayString *data = new flxDataArrayString;
        bool bstatus = (my_object->*_getter)(data);

        if (!bstatus)
        {
            delete data;
            return nullptr;
        }

        return data;
    }
};

//-----------------------------------------------------------------------------------

template <class T, class Object, void (Object::*_setter)(T const &)>
class _flxParameterIn : public flxParameterIn, public _flxDataIn<T>
{
    Object *my_object; // Pointer to the containing object

  public:
    _flxParameterIn() : my_object(0)
    {
    }

    // Limit data range
    _flxParameterIn(T min, T max)
    {
        _flxDataIn<T>::setDataLimitRange(min, max);
    }
    // Limit data set
    _flxParameterIn(std::initializer_list<std::pair<const std::string, T>> limitSet)
    {
        _flxDataIn<T>::addDataLimitValidValue(limitSet);
    }

    //---------------------------------------------------------------------------------
    flxDataType_t type()
    {
        return _flxDataIn<T>::type();
    };
    //---------------------------------------------------------------------------------
    // to register the property - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     property_obj(this);
    //
    // This allows the property to add itself to the containing objects list of
    // properties.
    //
    // Also the containing object is needed to call the getter/setter methods on that object

    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "ParamIn: invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }

    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }
    //---------------------------------------------------------------------------------
    void set(T const &value)
    {

        if (!my_object)
        {
            flxLogM_E(kMsgParentObjNotSet, "input parameter");
            return;
        }

        (my_object->*_setter)(value);
    }

    //---------------------------------------------------------------------------------
    // set -> parameter(value)
    void operator()(T const &value)
    {
        set(value);
    };
    //---------------------------------------------------------------------------------
    // editValue()
    //
    // Send the property value to the passed in editor for -- well -- editing
    flxEditResult_t editValue(flxDataEditor &theEditor)
    {

        T value = 0;

        bool bSuccess = theEditor.editField(value);

        if (bSuccess) // success
        {
            // do we have a dataLimit set, and if so are we in limits?
            if (!_flxDataIn<T>::isValueValid(value))
                return flxEditOutOfRange;

            set(value);
        }

        return bSuccess ? flxEditSuccess : flxEditFailure;
    }
    bool setValue(flxDataVariable &value)
    {

        if (value.type == type())
        {
            T c;
            set(value.get(c));
            return true;
        }
        return false;
    };

    flxDataLimit *dataLimit(void)
    {
        return _flxDataIn<T>::dataLimit();
    }
};

// Define by type

// bool
template <class Object, void (Object::*_setter)(bool const &)>
using flxParameterInBool = _flxParameterIn<bool, Object, _setter>;

template <class Object, void (Object::*_setter)(int8_t const &)>
using flxParameterInInt8 = _flxParameterIn<int8_t, Object, _setter>;

template <class Object, void (Object::*_setter)(int16_t const &)>
using flxParameterInInt16 = _flxParameterIn<int16_t, Object, _setter>;

template <class Object, void (Object::*_setter)(int32_t const &)>
using flxParameterInInt32 = _flxParameterIn<int32_t, Object, _setter>;

template <class Object, void (Object::*_setter)(uint8_t const &)>
using flxParameterInUInt8 = _flxParameterIn<uint8_t, Object, _setter>;

template <class Object, void (Object::*_setter)(uint16_t const &)>
using flxParameterInUInt16 = _flxParameterIn<uint16_t, Object, _setter>;

template <class Object, void (Object::*_setter)(uint32_t const &)>
using flxParameterInUInt32 = _flxParameterIn<uint32_t, Object, _setter>;

template <class Object, void (Object::*_setter)(float const &)>
using flxParameterInFloat = _flxParameterIn<float, Object, _setter>;

template <class Object, void (Object::*_setter)(double const &)>
using flxParameterInDouble = _flxParameterIn<double, Object, _setter>;

// strings are special.
template <class Object, void (Object::*_setter)(std::string const &)>
class flxParameterInString : public flxParameterIn, _flxDataInString
{
    Object *my_object; // Pointer to the containing object

  public:
    flxParameterInString() : my_object(0)
    {
    }

    flxParameterInString(Object *me) : my_object(me)
    {
    }
    //---------------------------------------------------------------------------------
    flxDataType_t type()
    {
        return _flxDataInString::type();
    };

    //---------------------------------------------------------------------------------
    // to register the property - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     property_obj(this);
    //
    // This allows the property to add itself to the containing objects list of
    // properties.
    //
    // Also the containing object is needed to call the getter/setter methods on that object
    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "ParamIn: invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }

    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }
    //---------------------------------------------------------------------------------
    void set(std::string const &value)
    {

        if (!my_object)
        {
            flxLogM_E(kMsgParentObjNotSet, "input parameter");
            return;
        }

        (my_object->*_setter)(value);
    }

    //---------------------------------------------------------------------------------
    // set -> parameter(value)
    void operator()(std::string const &value)
    {
        set(value);
    };
    //---------------------------------------------------------------------------------
    // editValue()
    //
    // Send the property value to the passed in editor for -- well -- editing
    flxEditResult_t editValue(flxDataEditor &theEditor)
    {

        std::string value = "";

        bool bSuccess = theEditor.editField(value);

        if (bSuccess) // success
            set(value);

        return bSuccess ? flxEditSuccess : flxEditFailure;
    }
    flxDataLimit *dataLimit(void)
    {
        return nullptr;
    }

    bool setValue(flxDataVariable &value)
    {

        if (value.type == type())
        {
            std::string c;
            set(value.get(c));
            return true;
        }
        return false;
    };
};

// Need a wedge class to make it easy to cast to a void outside of the template

class flxParameterInVoidType : public flxParameterIn
{
  public:
    flxParameterInVoidType() : prompt{true}
    {
    }

    virtual void set(void) = 0;
    bool prompt;
};

// VOID input parameter -- function call, no params
template <class Object, void (Object::*_setter)()> class flxParameterInVoid : public flxParameterInVoidType
{
    Object *my_object; // Pointer to the containing object

  public:
    flxParameterInVoid() : my_object(0)
    {
    }

    flxParameterInVoid(Object *me) : my_object(me)
    {
    }
    //---------------------------------------------------------------------------------
    flxDataType_t type()
    {
        return flxTypeNone;
    };
    //---------------------------------------------------------------------------------
    // to register the property - set the containing object instance
    // Normally done in the containing objects constructor.
    // i.e.
    //     property_obj(this);
    //
    // This allows the property to add itself to the containing objects list of
    // properties.
    //
    // Also the containing object is needed to call the getter/setter methods on that object
    void operator()(Object *obj)
    {
        // my_object must be derived from _flxParameterContainer
        static_assert(std::is_base_of<_flxParameterContainer, Object>::value, "ParamIn: invalid object");

        my_object = obj;
        assert(my_object);

        if (my_object)
            my_object->addParameter(this);
    }
    void operator()(Object *obj, const char *name)
    {
        // set the name of the property on init
        if (name)
            setName(name);

        // cascade to other version of method
        (*this)(obj);
    }

    void operator()(Object *obj, const char *name, const char *desc)
    {
        // Description of the object
        if (desc)
            setDescription(desc);

        // cascade to other version of method
        (*this)(obj, name);
    }

    void operator()(Object *obj, const char *name, const char *desc, flxParamValueType_t vtype)
    {
        // Value type
        setValueType(vtype);

        // cascade to other version of method
        (*this)(obj, name, desc);
    }

    //---------------------------------------------------------------------------------
    void set()
    {
        if (!my_object)
        {
            flxLogM_E(kMsgParentObjNotSet, "input parameter");
            return;
        }

        (my_object->*_setter)();
    }

    //---------------------------------------------------------------------------------
    // set -> parameter(value)
    void operator()()
    {
        set();
    };
    //---------------------------------------------------------------------------------
    // editValue()
    //
    // there is nothing to edit - this method just supports the interface
    flxEditResult_t editValue(flxDataEditor &theEditor)
    {
        return flxEditSuccess;
    };
    flxDataLimit *dataLimit(void)
    {
        return nullptr;
    };
    bool setValue(flxDataVariable &value)
    {
        return true;
    };
};
// Handy macros to "register attributes (props/params)"

// If the user doesn't supply a unique name or desc - use the object name/prop var name for the name

// Use some macro magic to determine which actual call to make based on the number of passed in
// parameters..
#define _spGetRegAttributeMacro(_1, _2, _3, _4, _NAME_, ...) _NAME_
#define flxRegister(...)                                                                                               \
    _spGetRegAttributeMacro(__VA_ARGS__, flxRegisterValueType, flxRegisterDesc, flxRegisterName,                       \
                            flxRegisterObj)(__VA_ARGS__)

#define flxRegisterObj(_obj_name_) _obj_name_(this, #_obj_name_)

// User provided Name
#define flxRegisterName(_obj_name_, _name_) _obj_name_(this, _name_)

// User provided Name and description
#define flxRegisterDesc(_obj_name_, _name_, _desc_) _obj_name_(this, _name_, _desc_)

// For parameters - user provided value type
#define flxRegisterValueType(_obj_name_, _name_, _desc_, _type_) _obj_name_(this, _name_, _desc_, _type_)

// Define a object type that supports parameter lists (input and output)
class flxOperation : public flxObject, public _flxParameterContainer
{
  public:
    virtual flxTypeID getType(void)
    {
        return (flxTypeID) nullptr;
    }

    /// @brief Virtual method called to run the operation - called before data is retrieved
    virtual bool execute(void)
    {
        return true;
    }

    virtual bool onSave(flxStorageBlock *stBlk)
    {
        if (!stBlk)
            return false;

        //  need to stash our parameter enable flags.
        flxParameterOutList outParams = getOutputParameters();

        for (auto param : outParams)
        {
            if (!stBlk->write(param->name(), param->enabled()))
                flxLog_E(F("Error saving enabled flag for %s - parameter %s"), name(), param->name());
        }

        return flxObject::onSave(stBlk);
    }

    virtual bool onRestore(flxStorageBlock *stBlk)
    {
        if (!stBlk)
            return false;

        // se need to restore our parameter enable flags.
        flxParameterOutList outParams = getOutputParameters();

        bool isEnabled;
        for (auto param : outParams)
        {
            if (stBlk->read(param->name(), isEnabled))
                param->setEnabled(isEnabled);
        }

        return flxObject::onRestore(stBlk);
    }
};

using flxOperationContainer = flxContainer<flxOperation *>;
//-----------------------------------------
// Spark Actions
//
// flxAction - just to enable grouping of actions
class flxAction : public flxOperation
{
  public:
    virtual bool initialize(void)
    {
        return true;
    }
};
// Container for actions
using flxActionContainer = flxContainer<flxAction *>;

// For subclasses
template <typename T> class flxActionType : public flxAction
{
  public:
    // ---------------------------------------------------------------
    // Typing system for actions
    //
    // Defines a type specific static method - so can be called outside
    // of an instance.
    //
    // The typeID is determined by hashing the name of the class.
    // This way the type ID is consistent across invocations

    static flxTypeID type(void)
    {
        static flxTypeID _myTypeID = flxGetClassTypeID<T>();

        return _myTypeID;
    }

    // ---------------------------------------------------------------
    // Return the type ID of this
    flxTypeID getType(void)
    {
        return type();
    }
};

// KDB - Temporary ...

template <typename T> class flxSystemType : public flxActionType<T>
{

  public:
    // setup and life-cycle of the file system interface
    virtual bool initialize() = 0;

    // Power interface
    virtual void setPower(bool powerOn) = 0;
    void powerOn(void)
    {
        setPower(true);
    }
    void powerOff()
    {
        setPower(false);
    }
    virtual bool power(void) = 0;
};
// End - flxCoreParam.h
