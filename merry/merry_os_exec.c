#include "internals/merry_os_exec.h"

_os_exec_(halt)
{
    // here we have to stop the core first
    // since the core is waiting, we don't have to worry about mutex locks.
    // since the core's decoder won't mess with other fields of the core, we can freely make changes
    // _llog_(_OS_, "Request", " Fulfilling the halt request: Requester %d", request->id);
    os->cores[request->id]->stop_running = mtrue; // this will automatically halt the core's decoder as well
    if (os->core_count == 1)                      // I am an idiot
    {
        // we had only one core to begin with then stop any further execution
        os->stop = mtrue;
        // the core that makes this request should have the return value in Ma register
        os->ret = os->cores[request->id]->registers[Ma];
    }
    printf("Halting.\n"); /// TODO: remove this
    // _llog_(_OS_, "REQ_SUCCESS", "Halt request successfully fulfilled for core ID %lu", os->cores[request->id]->core_id);
    return RET_SUCCESS; // for mitigating compiler's warning
}

_os_exec_(new_core)
{
    // generate a new core
    // _llog_(_OS_, "Request", " Creating a new core: Requester %d", request->id);
    if (merry_os_add_core() == RET_FAILURE)
    {
        // let the core know that its request was a failure
        os->cores[request->id]->registers[Ma] = 1; // Ma should contain the address and it will be updated with the result of the request
        // _llog_(_OS_, "Request", "Creation of a new core failed: Requester %d", request->id);
    }
    else
    {
        os->cores[request->id]->registers[Ma] = merry_os_boot_core(os->core_count - 1, os->cores[request->id]->registers[Ma]);
        // _llog_(_OS_, "Request", " Successfully Created a new core: Requester %d", request->id);
    }
    return RET_SUCCESS; // for now
}

_os_exec_(dynl)
{
    // the address to the name of the library must be in the Ma register
    // if the name is not and it is invalidly placed, the host will throw a segfault
    mbptr_t name = merry_dmemory_get_byte_address(os->data_mem, os->cores[request->id]->registers[Ma]);
    if (name == RET_NULL)
    {
        merry_requestHdlr_panic(os->data_mem->error);
        return RET_FAILURE;
    }
    if (merry_loader_loadLib(name, &os->cores[request->id]->registers[Mb]) == mfalse)
    {
        merry_requestHdlr_panic(MERRY_DYNL_FAILED);
        return RET_FAILURE;
    }
    return RET_SUCCESS;
}

_MERRY_ALWAYS_INLINE_ _os_exec_(dynul)
{
    merry_loader_unloadLib(os->cores[request->id]->registers[Mb]);
    return RET_SUCCESS;
}

_os_exec_(dyncall)
{
    dynfunc_t function;
    mqptr_t param = merry_dmemory_get_qword_address(os->data_mem, os->cores[request->id]->registers[Mc]);
    mbptr_t func_name = merry_dmemory_get_byte_address(os->data_mem, os->cores[request->id]->registers[Ma]);
    if (param == NULL || func_name == NULL)
    {
        merry_requestHdlr_panic(MERRY_DYNCALL_FAILED);
        return RET_FAILURE;
    }
    if ((function = merry_loader_getFuncSymbol(os->cores[request->id]->registers[Mb], func_name)) == RET_NULL)
    {
        merry_requestHdlr_panic(MERRY_DYNCALL_FAILED);
        return RET_FAILURE;
    }
    os->cores[request->id]->registers[Ma] = function(param);
    return RET_SUCCESS;
}

/// NOTE: It is the program's job to not mess with the file handle. The VM might crash or undefined behaviour could occur. Since there is no direct way for the VM to know that the file handle
// is indeed valid, it is the program's job to keep the handle safe. The same can be applied to the handle returned by dynl

_os_exec_(fopen)
{
    // the address to the filename should be in Ma
    // the file opening mode should be in the lower 3 bits of the Mb register
    // if incorrect mode is specified, the file will be opened in read mode by default
    // the handle will be returned in the Mb register and the return value in the Ma register
    // the VM won't exit on open failure
    // the filename must be null terminated
    mbptr_t file_name = merry_dmemory_get_byte_address(os->data_mem, os->cores[request->id]->registers[Ma]);
    mstr_t open_mode = _openmode_((os->cores[request->id]->registers[Mb] & 0b111));
    FILE *temp = fopen(file_name, open_mode);
    if (temp == NULL)
    {
        os->cores[request->id]->registers[Mb] = 0; // representing NULL
        os->cores[request->id]->registers[Ma] = 1; // representing Failure
        return RET_FAILURE;
    }
    else
    {
        os->cores[request->id]->registers[Mb] = (mqword_t)temp; // store the handle
        os->cores[request->id]->registers[Ma] = 0;              // representing success
    }
    return RET_SUCCESS;
}

_os_exec_(fclose)
{
    // the handle to the file to close must be in the Mb register
    register mqword_t handle = os->cores[request->id]->registers[Mb];
    if ((mqptr_t)handle == NULL)
    {
        // if it is 0 then it is not a good practice to close a NULL file
        merry_requestHdlr_panic(MERRY_FILEHANDLE_NULL);
        return RET_FAILURE;
    }
    fclose((FILE *)handle);
    return RET_SUCCESS;
}

_os_exec_(fread)
{
    // if the file handle is NULL, error is thrown
    // The file handle is in the Mb register
    // The address to store the read contents should be in the Ma register
    // The number of bytes to read should be in the Mc register
    // The number of bytes read will be in the Ma register
    register mqword_t handle = os->cores[request->id]->registers[Mb];
    if ((mqptr_t)handle == NULL)
    {
        // if it is 0 then it is not a good practice to close a NULL file
        merry_requestHdlr_panic(MERRY_FILEHANDLE_NULL);
        return RET_FAILURE;
    }
    mbptr_t store_in = merry_dmemory_get_byte_address(os->data_mem, os->cores[request->id]->registers[Ma]);
    register msize_t bytes_to_read = os->cores[request->id]->registers[Mc];
    os->cores[request->id]->registers[Ma] = fread(store_in, 1, bytes_to_read, (FILE *)handle);
    return RET_SUCCESS;
}

_os_exec_(fwrite)
{
    // exactly the same but write is performed instead
    register mqword_t handle = os->cores[request->id]->registers[Mb];
    if ((mqptr_t)handle == NULL)
    {
        // if it is 0 then it is not a good practice to close a NULL file
        merry_requestHdlr_panic(MERRY_FILEHANDLE_NULL);
        return RET_FAILURE;
    }
    mbptr_t to_write = merry_dmemory_get_byte_address(os->data_mem, os->cores[request->id]->registers[Ma]);
    register msize_t bytes_to_read = os->cores[request->id]->registers[Mc];
    os->cores[request->id]->registers[Ma] = fwrite(to_write, 1, bytes_to_read, (FILE *)handle);
    return RET_SUCCESS;
}

_os_exec_(feof)
{
    // handle in Mb register
    // nonzero in Ma if eof else 0
    register mqword_t handle = os->cores[request->id]->registers[Mb];
    if ((mqptr_t)handle == NULL)
    {
        // if it is 0 then it is not a good practice to close a NULL file
        merry_requestHdlr_panic(MERRY_FILEHANDLE_NULL);
        return RET_FAILURE;
    }
    os->cores[request->id]->registers[Ma] = feof((FILE *)handle);
    return RET_SUCCESS;
}