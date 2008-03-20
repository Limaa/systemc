/*****************************************************************************

  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2007 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License Version 3.0 (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.systemc.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.

*****************************************************************************/


#ifndef __TLM_GP_H__
#define __TLM_GP_H__

#include <systemc>
#include "tlm_array.h"

namespace tlm {

//---------------------------------------------------------------------------
// Classes and helper functions for the extension mechanism
//---------------------------------------------------------------------------
// Helper function:
inline unsigned int max_num_extensions(bool increment=false)
{
    static unsigned int max_num = 0;
    if (increment) ++max_num;
    return max_num;
}

// This class can be used for storing pointers to the extension classes, used
// in tlm_generic_payload:
class tlm_extension_base
{
public:
    virtual tlm_extension_base* clone() const = 0;
    virtual ~tlm_extension_base() {}
protected:
    static unsigned int register_extension()
    {
        return (max_num_extensions(true) - 1);
    };
};

// Base class for all extension classes, derive your extension class in
// the following way:
// class my_extension : public tlm_extension<my_extension> { ...
// This triggers proper extension registration during C++ static
// contruction time. my_extension::ID will hold the unique index in the
// tlm_generic_payload::m_extensions array.
template <typename T>
class tlm_extension : public tlm_extension_base
{
public:
    virtual tlm_extension_base* clone() const = 0;
    virtual ~tlm_extension() {}
    const static unsigned int ID;
};

template <typename T>
const
unsigned int tlm_extension<T>::ID = tlm_extension_base::register_extension();

//---------------------------------------------------------------------------
// enumeration types
//---------------------------------------------------------------------------
enum tlm_command {
    TLM_READ_COMMAND,
    TLM_WRITE_COMMAND,
    TLM_IGNORE_COMMAND
};

enum tlm_response_status {
    TLM_OK_RESPONSE = 1,
    TLM_INCOMPLETE_RESPONSE = 0,
    TLM_GENERIC_ERROR_RESPONSE = -1,
    TLM_ADDRESS_ERROR_RESPONSE = -2,
    TLM_COMMAND_ERROR_RESPONSE = -3,
    TLM_BURST_ERROR_RESPONSE = -4,
    TLM_BYTE_ENABLE_ERROR_RESPONSE = -5
};

//---------------------------------------------------------------------------
// The generic payload class:
//---------------------------------------------------------------------------
class tlm_generic_payload {
public:

    //---------------
    // Constructors
    //---------------
    
    // Default constructor
    tlm_generic_payload() 
        : m_command(TLM_IGNORE_COMMAND)
        , m_address(0)
        , m_data(0)
        , m_length(0)
        , m_response_status(TLM_INCOMPLETE_RESPONSE)
        , m_byte_enable(0)
        , m_byte_enable_length(0)
        , m_streaming_width(0)
        , m_extensions(max_num_extensions())
        , m_dmi(false)
    {
    }
    
    // Copy constructor
    tlm_generic_payload(const tlm_generic_payload& x)
        : m_command(x.get_command())
        , m_address(x.get_address())
        , m_data(x.get_data_ptr())
        , m_length(x.get_data_length())
        , m_response_status(x.get_response_status())
        , m_byte_enable(x.get_byte_enable_ptr())
        , m_byte_enable_length(x.get_byte_enable_length())
        , m_streaming_width(x.get_streaming_width())
        , m_extensions(max_num_extensions())
        , m_dmi(x.get_dmi_allowed())
    {
        // copy all extensions
        for(unsigned int i=0; i<m_extensions.size(); i++)
        {
            m_extensions[i] = x.get_extension(i);
        }
    }

    // Assignment operator
    tlm_generic_payload& operator= (const tlm_generic_payload& x)
    {
        m_command =            x.get_command();
        m_address =            x.get_address();
        m_data =               x.get_data_ptr();
        m_length =             x.get_data_length();
        m_response_status =    x.get_response_status();
        m_byte_enable =        x.get_byte_enable_ptr();
        m_byte_enable_length = x.get_byte_enable_length();
        m_streaming_width =    x.get_streaming_width();
        m_dmi =                x.get_dmi_allowed();

        // extension copy: all extension arrays must be of equal size by
        // construction (i.e. it must either be constructed after C++
        // static construction time, or the resize_extensions() method must
        // have been called prior to using the object)
        for(unsigned int i=0; i<m_extensions.size(); i++)
        {
            m_extensions[i] = x.get_extension(i);
        }
        return (*this);
    }

    // non-virtual deep-copying of the object
    tlm_generic_payload* deep_copy() const
    {
        tlm_generic_payload* tmp = new tlm_generic_payload(*this);
        // deep copy data
        if(m_data && m_length)
        {
            unsigned char* tmp_data = new unsigned char[m_length];
            tmp->set_data_ptr(tmp_data);
            for(unsigned int i=0; i<m_length; i++)
            {
                tmp_data[i] = m_data[i];
            }
        }
        // deep copy byte enables
        if(m_byte_enable && m_byte_enable_length)
        {
            unsigned char* tmp_byte_enable = new unsigned char[m_byte_enable_length];
            tmp->set_byte_enable_ptr(tmp_byte_enable);
            tmp->set_byte_enable_length(m_byte_enable_length);
            for (unsigned int i=0; i<m_byte_enable_length; i++)
            {
                tmp_byte_enable[i] = m_byte_enable[i];
            }
        }
        // deep copy extensions
        for(unsigned int i=0; i<m_extensions.size(); i++)
        {
            if(m_extensions[i])
            {
                tmp->set_extension(i, m_extensions[i]->clone());
            }
        }
        return tmp;
    }

    //--------------
    // Destructor
    //--------------
    virtual ~tlm_generic_payload() {}
       
    //----------------
    // API (including setters & getters)
    //---------------

    // Command related method
    inline bool                 is_read() const {return (m_command == TLM_READ_COMMAND);}
    inline void                 set_read() {m_command = TLM_READ_COMMAND;}
    inline bool                 is_write() const {return (m_command == TLM_WRITE_COMMAND);}
    inline void                 set_write() {m_command = TLM_WRITE_COMMAND;}
    inline tlm_command          get_command() const {return m_command;}
    inline void                 set_command(const tlm_command command) {m_command = command;}
    
    // Address related methods
    inline sc_dt::uint64        get_address() const {return m_address;}
    inline void                 set_address(const sc_dt::uint64 address) {m_address = address;}
    
    // Data related methods
    inline unsigned char*       get_data_ptr() const {return m_data;}
    inline void                 set_data_ptr(unsigned char* data) {m_data = data;}
    
    // Transaction length (in bytes) related methods
    inline unsigned int         get_data_length() const {return m_length;}
    inline void                 set_data_length(const unsigned int length) {m_length = length;}
    
    // Response status related methods
    inline bool                 is_response_ok() const {return (m_response_status > 0);}
    inline bool                 is_response_error() const {return (m_response_status <= 0);}
    inline tlm_response_status  get_response_status() const {return m_response_status;}
    inline void                 set_response_status(const tlm_response_status response_status)
        {m_response_status = response_status;}  
    inline std::string          get_response_string() const
    {
        switch(m_response_status)
        {
        case TLM_OK_RESPONSE:            return "TLM_OK_RESPONSE"; break;
        case TLM_INCOMPLETE_RESPONSE:    return "TLM_INCOMPLETE_RESPONSE"; break;
        case TLM_GENERIC_ERROR_RESPONSE: return "TLM_GENERIC_ERROR_RESPONSE"; break;
        case TLM_ADDRESS_ERROR_RESPONSE: return "TLM_ADDRESS_ERROR_RESPONSE"; break;
        case TLM_COMMAND_ERROR_RESPONSE: return "TLM_COMMAND_ERROR_RESPONSE"; break;
        case TLM_BURST_ERROR_RESPONSE:   return "TLM_BURST_ERROR_RESPONSE"; break;
        case TLM_BYTE_ENABLE_ERROR_RESPONSE: return "TLM_BYTE_ENABLE_ERROR_RESPONSE"; break;	
        }
        return "TLM_UNKNOWN_RESPONSE";
    }
    
    // Streaming related methods
    inline unsigned int         get_streaming_width() const {return m_streaming_width;}
    inline void                 set_streaming_width(const unsigned int streaming_width) {m_streaming_width = streaming_width; }
        
    // Byte enable related methods
    inline unsigned char*       get_byte_enable_ptr() const {return m_byte_enable;}
    inline void                 set_byte_enable_ptr(unsigned char* byte_enable){m_byte_enable = byte_enable;}
    inline unsigned int         get_byte_enable_length() const {return m_byte_enable_length;}
    inline void                 set_byte_enable_length(const unsigned int byte_enable_length){m_byte_enable_length = byte_enable_length;}
    
    /* --------------------------------------------------------------------- */
    /* Generic Payload attributes:                                           */
    /* --------------------------------------------------------------------- */
    /* - m_command         : Type of transaction. Three values supported:    */
    /*                       - TLM_WRITE_COMMAND                             */
    /*                       - TLM_READ_COMMAND                              */
    /*                       - TLM_IGNORE_COMMAND                            */
    /* - m_address         : Transaction base address (byte-addressing).     */
    /* - m_data            : When m_command = TLM_WRITE_COMMAND contains a   */
    /*                       pointer to the data to be written in the target.*/
    /*                       When m_command = TLM_READ_COMMAND contains a    */
    /*                       pointer where to copy the data read from the    */
    /*                       target.                                         */
    /* - m_length          : Total number of bytes of the transaction.       */
    /* - m_response_status : This attribute indicates whether an error has   */
    /*                       occurred during the transaction.                */
    /*                       Values supported are:                           */
    /*                       - TLM_OK_RESP                                   */
    /*                       - TLM_INCOMPLETE_RESP                           */
    /*                       - TLM_GENERIC_ERROR_RESP                        */
    /*                       - TLM_ADDRESS_ERROR_RESP                        */
    /*                       - TLM_COMMAND_ERROR_RESP                        */
    /*                       - TLM_BURST_ERROR_RESP                          */
    /*                       - TLM_BYTE_ENABLE_ERROR_RESP                    */
    /*                                                                       */
    /* - m_byte_enable     : It can be used to create burst transfers where  */
    /*                    the address increment between each beat is greater */
    /*                    than the word length of each beat, or to place     */
    /*                    words in selected byte lanes of a bus.             */
    /* - m_byte_enable_length : For a read or a write command, the target    */
    /*                    interpret the byte enable length attribute as the  */
    /*                    number of elements in the bytes enable array.      */
    /* - m_streaming_width  :                                                */
    /* --------------------------------------------------------------------- */

private:

    tlm_command          m_command;
    sc_dt::uint64        m_address;
    unsigned char*       m_data;
    unsigned int         m_length;
    tlm_response_status  m_response_status;
    unsigned char*       m_byte_enable;
    unsigned int         m_byte_enable_length;
    unsigned int         m_streaming_width;
    
public:
   
    /* --------------------------------------------------------------------- */
    /* Dynamic extension mechanism:                                          */
    /* --------------------------------------------------------------------- */
    /* The extension mechanism is intended to enable initiator modules to    */
    /* optionally and transparently add data fields to the                   */
    /* tlm_generic_payload. Target modules are free to check for extensions  */
    /* and may or may not react to the data in the extension fields. The     */
    /* definition of the extensions' semantics is solely in the              */
    /* responsibility of the user.                                           */
    /*                                                                       */
    /* The following rules apply:                                            */
    /*                                                                       */
    /* - Every extension class must be derived from tlm_extension, e.g.:     */
    /*     class my_extension : public tlm_extension<my_extension> { ... }   */
    /*                                                                       */
    /* - A tlm_generic_payload object should be constructed after C++        */
    /*   static initialization time. This way it is guaranteed that the      */
    /*   extension array is of sufficient size to hold all possible          */
    /*   extensions. Alternatively, the initiator module can enforce a valid */
    /*   extension array size by calling the resize_extensions() method      */
    /*   once before the first transaction with the payload object is        */
    /*   initiated.                                                          */
    /*                                                                       */
    /* - Initiators should use the the set_extension(e) or clear_extension(e)*/
    /*   methods for manipulating the extension array. The type of the       */
    /*   argument must be a pointer to the specific registered extension     */
    /*   type (my_extension in the above example) and is used to             */
    /*   automatically locate the appropriate index in the array.            */
    /*                                                                       */
    /* - Targets can check for a specific extension by calling               */
    /*   get_extension(e). e will point to zero if the extension is not      */
    /*   present.                                                            */
    /*                                                                       */
    /* --------------------------------------------------------------------- */

    // Stick the pointer to an extension into the vector, return the
    // previous value:
    template <typename T> T* set_extension(T* ext)
    {
        resize_extensions();
        T* tmp = static_cast<T*>(m_extensions[T::ID]);
        m_extensions[T::ID] = static_cast<tlm_extension_base*>(ext);
        return tmp;
    }
    // non-templatized version with manual index:
    tlm_extension_base* set_extension(unsigned int index,
                                      tlm_extension_base* ext)
    {
        resize_extensions();
        tlm_extension_base* tmp = m_extensions[index];
        m_extensions[index] = ext;
        return tmp;
    }

    // Check for an extension, ext will point to 0 if not present
    template <typename T> void get_extension(T*& ext) const
    {
        ext = static_cast<T*>(m_extensions[T::ID]);
    }
    template <typename T> T* get_extension() const
    {
        T *ext;
        get_extension(ext);
        return ext;
    }
    // Non-templatized version:
    tlm_extension_base* get_extension(unsigned int index) const
    {
        return m_extensions[index];
    }

    // Clear extension, the argument is needed to find the right index:
    template <typename T> void clear_extension(const T* ext)
    {
        resize_extensions();
        m_extensions[T::ID] = static_cast<tlm_extension_base*>(0);
    }
    template <typename T> void clear_extension()
    {
        resize_extensions();
        m_extensions[T::ID] = static_cast<tlm_extension_base*>(0);
    }
    // Non-templatized version with manual index
    void clear_extension(unsigned int index)
    {
        if (index < m_extensions.size())
        {
            m_extensions[index] = static_cast<tlm_extension_base*>(0);
        }
    }

    // Make sure the extension array is large enough. Can be called once by
    // an initiator module (before issuing the first transaction) to make
    // sure that the extension array is of correct size. This is only needed
    // if the initiator cannot guarantee that the generic payload object is
    // allocated after C++ static construction time.
    void resize_extensions()
    {
        m_extensions.expand(max_num_extensions());
    }

private:
    tlm_array<tlm_extension_base*> m_extensions;

public:
  // This is the "DMI-hint" a slave can set this to true if it
  // wants to indicate that a DMI request would be supported:
  void set_dmi_allowed(bool dmiAllowed) { m_dmi = dmiAllowed; }
  bool get_dmi_allowed() const { return m_dmi; }

private:
  bool m_dmi;
};

} // namespace tlm

#endif /* __TLM_GP_H__ */
