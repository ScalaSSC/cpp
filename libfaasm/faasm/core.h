#ifndef FAASMC_CORE_H
#define FAASMC_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "faasm/memory.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define BYTES(arr) reinterpret_cast<uint8_t*>(arr)

    /**
     * Definition of a Faasm function pointer
     */
    typedef int (*FaasmFuncPtr)();

    /**
     * Gets the size of the state at the given key
     */
    size_t faasmReadStateSize(const char* key);

    /**
     * Reads the full state at the given key
     */
    long faasmReadState(const char* key, unsigned char* buffer, long bufferLen);

    /**
     * Reads append-only state
     */
    void faasmReadAppendedState(const char* key,
                                uint8_t* buffer,
                                long bufferLen,
                                long nElems);

    /**
     * Reads the full state and returns a direct pointer
     */
    uint8_t* faasmReadStatePtr(const char* key, long totalLen);

    /**
     * Reads a chunk of state at the given key and offset
     */
    void faasmReadStateOffset(const char* key,
                              long totalLen,
                              long offset,
                              uint8_t* buffer,
                              long bufferLen);

    /**
     * Reads a chunk of state and returns a direct pointer
     */
    uint8_t* faasmReadStateOffsetPtr(const char* key,
                                     long fullLen,
                                     long offset,
                                     long len);

    /**
     * Overwrites the state at the given key
     */
    void faasmWriteState(const char* key, const uint8_t* data, long dataLen);

    /**
     * Writes to append-only state
     */
    void faasmAppendState(const char* key, const uint8_t* data, long dataLen);

    /**
     * Clears the appended state
     */
    void faasmClearAppendedState(const char* key);

    /**
     * Writes a chunk of state at the given key and offset
     */
    void faasmWriteStateOffset(const char* key,
                               long totalLen,
                               long offset,
                               const uint8_t* data,
                               long dataLen);

    /**
     * Writes the given file contents to a file
     */
    unsigned long faasmWriteStateFromFile(const char* key,
                                          const char* filePath);

    /**
     * Mark the whole value as dirty
     */
    void faasmFlagStateDirty(const char* key, long totalLen);

    /**
     * Mark a segment as dirty
     */
    void faasmFlagStateOffsetDirty(const char* key,
                                   long totalLen,
                                   long offset,
                                   long len);

    /**
     * Forces a push of state
     */
    void faasmPushState(const char* key);

    /**
     * Forces a push of any partial state updates
     */
    void faasmPushStatePartial(const char* key);

    /**
     * Forces a push of any partial state updates with the given mask
     */
    void faasmPushStatePartialMask(const char* key, const char* maskKey);

    /**
     * Forces a pull of state
     */
    void faasmPullState(const char* key, long stateLen);

    /**
     * Acquires a read lock for the given state
     */
    void faasmLockStateRead(const char* key);

    /**
     * Releases a read lock for the given state
     */
    void faasmUnlockStateRead(const char* key);

    /**
     * Acquires a write lock for the given state
     */
    void faasmLockStateWrite(const char* key);

    /**
     * Releases a write lock for the given state
     */
    void faasmUnlockStateWrite(const char* key);

    /**
     * Returns the size of the input in bytes. Returns zero if none.
     * */
    long faasmGetInputSize();

    /**
     * Returns a pointer to the input data for this function
     */
    void faasmGetInput(uint8_t* buffer, long bufferLen);

    /**
     * Sets the given string as the output data for this function
     */
    void faasmSetOutput(const char* newOutput, long outputLen);

    /**
     * Chains a function with the given input data
     */
    unsigned int faasmChainNamed(const char* name,
                                 const uint8_t* inputData,
                                 long inputDataSize);

    /**
     * Chains a function with the given input data with the current messageIdx
     */
    unsigned int faasmChainNamedId(const char* name,
                                   const uint8_t* inputData,
                                   long inputDataSize,
                                   int idx);

    /**
     * Chains a function from this module with the given input data
     */
    unsigned int faasmChain(FaasmFuncPtr funcPtr,
                            const uint8_t* inputData,
                            long inputDataSize);

    /**
     * Chains a function from this module N times, passing the string as input
     * data
     */
    unsigned int faasmChainBatch(FaasmFuncPtr funcPtr,
                                 const char* inputData,
                                 int nFuncs);

    /**
     * Blocks waiting for the call
     */
    unsigned int faasmAwaitCall(unsigned int callId);

    /**
     * Gets the output from the given call into the buffer
     */
    unsigned int faasmAwaitCallOutput(unsigned int messageId,
                                      const char* output,
                                      long outputLen);

    /**
     * Returns the python user
     */
    char* faasmGetPythonUser();

    /**
     * Returns the python function
     */
    char* faasmGetPythonFunc();

    /**
     * Returns the python entrypoint
     */
    char* faasmGetPythonEntry();

    /**
     * Returns a 1 or 0 saying whether the conf flag is on or off
     */
    unsigned int getConfFlag(const char* key);

    /**
     * Requests that the runtime print a backtrace for the given depth
     */
    void faasmBacktrace(const int depth);

    /**
     * Creates a new function state
     */
    void faasmCreateFunctionState(const unsigned char* data, long dataLen);

    /**
     * Creates a new partitioned function state with its input and state keys.
     * It means the it is 'partition stateful function', which can be
     * partitioned by the input key, e.g. ID. The stateKey is used to store
     * the partitioned state.
     */
    void faasmCreatePartitionedFunctionState(const unsigned char* data,
                                             long dataLen,
                                             unsigned char* inputKey,
                                             unsigned char* stateKey);

    /**
     * Write the Function level state into State Storage
     */
    void faasmWriteFunctionState(const uint8_t* data, long dataLen);

    /**
     * Gets the size of the function state
     */
    size_t faasmReadFunctionStateSize();

    /**
     * Gets the size of the function state
     */
    size_t faasmReadFunctionStateSizeLock();

    /**
     * Reads the Function levl full state from State Storage
     */
    long faasmReadFunctionState(unsigned char* buffer, long bufferLen);

    /**
     * Read function state. InputKeys is used for partitioned stateful.
     */
    long faasmReadParitionedFunctionState(unsigned char* buffer,
                                          long bufferLen,
                                          unsigned char* inputKeys);

    /**
     * Read function state data. It returns a pointer of vector<uint8_t>. If
     * nullptr is returned, means this state is created but not initialized. In
     * the same time the data is locked.
     */
    uint8_t* faasmReadFunctionStatePtrLock();

    /**
     * Write the function state into state server. This function won't create
     * any functionstate object. It will also unlock the function after writing.
     */
    void faasmWriteFunctionStateUnlock(const uint8_t* data, long dataLen);

    /**********
     * The following three functions are used for partitioned stateful function.
     **********/

    /**
     * Read the size of the partitioned function state, with input keys. We only
     * retrieve the needed values.
     */
    size_t faasmReadPartitionedFunctionStateSizeLock(const char* inputKeys);

    /**
     * Read the the partitioned function state, with input keys. We only
     * retrieve the needed values. bufferLen is currently unused.
     */
    long faasmReadPartitionedFunctionState(unsigned char* buffer,
                                           long bufferLen,
                                           const char* inputKeys);

    /**
     * Write the updated partitioned function state back.
     */
    void faasmWritePartitionedFunctionStateUnlock(const uint8_t* data,
                                                  long dataLen);

    long faasmFunctionStateLock();

    void faasmFunctionStateUnlock();

    void faasmChainInvoke();

    // Macro for defining zygotes (a default fallback noop is provided)
    int __attribute__((weak)) _faasm_zygote();
#define FAASM_ZYGOTE() int _faasm_zygote()

#ifdef __cplusplus
}
#endif

#endif
