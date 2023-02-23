Adding Target-Specific Metadata
===============================

Targets often have specific ways of extracting the best performance
and it is necessary to store metadata as part of the binary so that 
knowledge from the compiler can be utilized at runtime. This tutorial 
explains how targets can hook into the metadata mechanism in ComputeAorta
and store custom metadata as part of the binary.

Defining the metadata struct and handler
----------------------------------------

Suppose that we want to store some property `X` for each kernel. To do this 
we first need to define a struct ``TargetXmetadata`` which extends ``GenericMetadata``
and add the property as a new member. 

.. code:: cpp

    struct TargetXmetadata : GenericMetadata {
      TargetXmetadata() = default;
      TargetXmetadata(std::string kernel_name, uint64_t local_memory_usage,
        PropXTy propX);
      PropXTy propX;
    };

Since the target specific class extends ``GenericMetadata`` we need to provide a 
constructor which includes the generic components: kernel name & local memory usage
and a value for our custom property. In the constructor's implementation 
you **must** forward the generic components to the ``GenericMetadata`` constructor. 

You also must define a handler class which will interact with the Metadata API 
and handle reading and writing of the metadata to the binary. A blueprint of such
a class is provided below. For details on the Metadata API please refer to its 
documentation :ref:`here <modules/metadata:metadata api>`.

.. code:: cpp

    class TargetXmetadataHandler : GenericMetadataHandler {
      virtual bool init(md_hooks *hooks, void *userdata) override {
        if(!GenericMetadataHandler::init(hooks, userdata)){
          return false;
        }
        /* Perform any custom initialization */
      }
      virtual bool finalize() override{
        /* Perform any needed finalization */
        return GenericMetadataHandler::finalize();
      }
      bool read(TargetXmetadata &md) {
        if(!GenericMetadataHandler::read(md)){
          return false;
        }
        /* Read propX from metadata API */
      }
      bool write(const TargetXmetadata &md) {
        if(!GenericMetadataHandler::write(md)){
          return false;
        }
        /* Write propX to the metadata API */
      }
    };

ComputeAorta itself already contains a similar specialization
for storing vectorization information in the binary. For inspiration on
how to implement the class you can refer to ``modules/metadata/include/metadata/handler/vectorize_info_metadata.h``
and ``modules/metadata/source/metadata/handler/vectorize_info_metadata.cpp``.

Define a Function Analysis
--------------------------

To allow the ``AddMetadataPass`` to correctly add the metadata to the binary
you **must** provide a Function Analysis pass, which will return a reference 
to a ``TargetXmetadata`` struct. It is therefore the responsibility of the 
analysis to extract the property `X` from the IR and encode it into the struct.

.. code:: cpp

    class TargetXmetadataAnalysis
        : public AnalysisInfoMixin<TargetXmetadataAnalysis> {
     friend AnalysisInfoMixin<TargetXmetadataAnalysis>;

     public:
      using Result = TargetXmetadata;
      TargetXmetadataAnalysis() = default;

      Result run(Function &Fn, FunctionAnalysisManager &AM) {
          /* Analyse IR to find property X*/
          return Result(...);
      }

      static llvm::StringRef name { return "TargetX metadata analysis"; }

     private:
      static AnalysisKey Key;
    };

You must also register the analysis in your target-specific ``pass_registry.def``
as a function analysis with an appropriate name.

Register the pass
-----------------

Finally, we can now register an ``AddMetadataPass`` which will take our analysis
and handler classes as template arguments. In your target-specific ``pass_registry.def``
you should add the following. 

.. note::
    We use a generic name here for the pass, you should select an appropriate name 
    based on your requirements.

.. code:: cpp

    MODULE_PASS("add-targetx-metadata", 
                compiler::utils::AddMetadataPass<TargetXmetadataAnalysis, 
                TargetXmetadataHandler>);

You can now add the pass at an appropriate point in the pipeline. Typically, this
will be at the end of the pipeline once all the information is known.

.. code:: cpp

    PM.addPass(compiler::utils::AddMetadataPass<TargetXmetadataAnalysis, 
               TargetXmetadataHandler>);
               
Reading back the metadata (mux side)
------------------------------------

The ``AddMetadataPass`` will place the serialized metadata bytes into a ``.notes`` section 
in the mux object file. To deserialize the metadata you should read the ``.notes`` section 
in your targets implementation of ``muxCreateExecutable`` and initialize a metadata API
context with a ``map()`` hook that will read the bytes from this section.

For a working example refer to ``readBinaryMetadata`` in 
``modules/mux/targets/host/source/metadata_hooks.cpp``, which deserializes the 
vectorized info metadata.
