Attaching External Custom Decoders    {#custom_decoders}
==================================

@brief A description of the C API external decoder interface.

Introduction
------------

An external custom decoder is one which decodes a CoreSight trace byte stream from a source other
than an ARM core which cannot be decoded by the standard builtin decoders within the library.

An example of this may be a trace stream from a DSP device.

The external decoder API allows a suitable decoder to be attached to the library and used in the 
same way as the built-in decoders. This means that the external decoder can be created and destroyed
using the decode tree API, and will integrate seamlessly with any ARM processor decoders that are part
of the same tree.

An external decoder will be required to provide two standard structures:-

- `ocsd_extern_dcd_fact_t` : this is a decoder "factory" that allows the creation of the custom decoders.
- `ocsd_extern_dcd_inst_t` : one of these structures exists for each instance of the custom decoder that is created.

These structures consist of data and function pointers to allow integration with the library infrastructure.

Registering A Decoder
---------------------

A single API function is provided to allow a decoder to be registered with the library by name. 

    ocsd_err_t ocsd_register_custom_decoder(const char *name, ocsd_extern_dcd_fact_t *p_dcd_fact);

This registers the custom decoder with the libary using the supplied name and factory structure.
Once registered, the standard API functions used with the built-in decoders will work with the custom decoder.

The Factory Structure
---------------------
This structure contains the interface that is registered with the library to allow the creation of custom decoder instances.

The mandatory functions that must be provided include:
- `fnCreateCustomDecoder`  : Creates a decoder. This function will fill in a `ocsd_extern_dcd_inst_t` structure for the decoder instance.
- `fnDestroyCustomDecoder` : Destroys the decoder. Takes the `decoder_handle` attribute of the instance structure.
- `fnGetCSIDFromConfig`    : Extracts the CoreSight Trace ID from the decoder configuration structure. 
                             May be called before the create function. The CSID is used as part of the creation process to 
                             attach the decoder to the correct trace byte stream.

`fnPacketToString` : This optional function will provide a human readable string from a protocol specific packet structure.

`protocol_id` : This is filled in when the decoder type is registered with the library. Used in some API 
                calls to specify the decoder protocol type.



The Decoder Instance Structure
------------------------------

This structure must be filled in by the `fnCreateCustomDecoder` function implmentation. 

There is a single mandatory function in this structure:

   `fnTraceDataIn` : the decoder must provide this as this is called by the library to provide the 
                     raw trace data to the decoder.

There are a number of optional callback functions. These allow the custom decoder to call into the library 
and use the same infrastructure as the builting decoders. The callbacks are registed with the custom decoder 
as the libary performs initialisation of connections. 

For example, the library will connect the error logging interface to all the decoders. The instance structure
contains the `fnRegisterErrLogCB` function pointer which will register two error logging callbacks with the custom 
decoder,  `fnLogErrorCB` and `fnLogMsgCB`. These can then be called by the custom decoder, and the errors and 
messages will appear in the same place as for the built-in decoders.

Where a custom decoder does not require or support the optional callbacks, then the registration function is set to 
0 in the structure. 






