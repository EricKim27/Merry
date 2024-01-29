#include "internals/merry_core.h"

MerryCore *merry_core_init(MerryMemory *inst_mem, MerryMemory *data_mem, msize_t id)
{
    // allocate a new core
    _llog_(_CORE_, "Intialization", "Intializing core with ID %lu", id);
    MerryCore *new_core = (MerryCore *)malloc(sizeof(MerryCore));
    // check if the core has been initialized
    if (new_core == RET_NULL)
        return RET_NULL;
    new_core->bp = 0; // initialize these registers to 0
    new_core->pc = 0;
    new_core->sp = 0;
    new_core->core_id = id;
    new_core->data_mem = data_mem;
    new_core->inst_mem = inst_mem;
    // initialize the locks and condition variables
    new_core->cond = merry_cond_init();
    if (new_core->cond == RET_NULL)
    {
        free(new_core);
        return RET_NULL;
    }
    new_core->lock = merry_mutex_init();
    if (new_core->lock == RET_NULL)
    {
        merry_cond_destroy(new_core->cond);
        free(new_core);
        return RET_NULL;
    }
    new_core->registers = (mqptr_t)malloc(sizeof(mqword_t) * REGR_COUNT);
    if (new_core->registers == RET_NULL)
        goto failure;
    new_core->stack_mem = (mqptr_t)_MERRY_MEM_GET_PAGE_(_MERRY_STACKMEM_BYTE_LEN_, _MERRY_PROT_DEFAULT_, _MERRY_FLAG_DEFAULT_);
    if (new_core->stack_mem == RET_NULL)
        goto failure;
    // new_core->should_wait = mtrue;   // should initially wait until said to run
    new_core->stop_running = mfalse; // this is set to false because as soon as the core is instructed to start/continue execution, it shouldn't stop and start immediately
    new_core->_is_private = mfalse;  // set to false by default
    new_core->decoder = merry_init_decoder(new_core);
    if (new_core->decoder == RET_NULL)
        goto failure;
    if ((new_core->decoder_thread = merry_thread_init()) == RET_NULL)
        goto failure;
    // we have done everything now
    return new_core;
failure:
    _log_(_CORE_, "FAILURE", "Core intialization failed");
    merry_core_destroy(new_core);
    return RET_NULL;
}

void merry_core_destroy(MerryCore *core)
{
    _llog_(_CORE_, "DESTROYING", "Destroying core with ID %lu", core->core_id);
    if (surelyF(core == NULL))
        return;
    if (surelyT(core->cond != NULL))
    {
        merry_cond_destroy(core->cond);
    }
    if (surelyT(core->lock != NULL))
    {
        merry_mutex_destroy(core->lock);
    }
    if (surelyT(core->registers != NULL))
    {
        free(core->registers);
    }
    if (surelyT(core->stack_mem != NULL))
    {
        if (_MERRY_MEM_GIVE_PAGE_(core->stack_mem, _MERRY_STACKMEM_BYTE_LEN_) == _MERRY_RET_GIVE_ERROR_)
        {
            // we failed here
            // This shouldn't happen unless the stack_mem variable was messed with and corrupted
            // This is the worst idea but we simply do nothing[OPPS UNSAFE? PFTT...WHAT COULD POSSIBLY GO WRONG?!]
            // nothing really, after the VM terminates, the OS will unmap all the resources that was being used and hence we becomes safe
            // While it would be interesting to know why this error happened, the cause probably didn't come from the VM itself since i am trying to write it as "SAFE" as possible
            // This may sound like a joke considering how absurd, unstructured, redundand, unsafe the code so far has been
        }
    }
    if (surelyT(core->decoder != NULL))
    {
        merry_destroy_decoder(core->decoder);
    }
    if (surelyT(core->decoder_thread != NULL))
    {
        merry_thread_destroy(core->decoder_thread);
    }
    core->data_mem = NULL;
    core->inst_mem = NULL;
    free(core);
}

mptr_t merry_runCore(mptr_t core)
{
    MerryCore *c = (MerryCore *)core;
    _llog_(_CORE_, "RUNNING", "Core ID %lu is now running", c->core_id);
    // the core is now in action
    // it's internal's are all initialized and it is ready to go
    // start the decoder
    if (merry_create_detached_thread(c->decoder_thread, &merry_decode, c->decoder) == RET_FAILURE)
    {
        // failed to start the decoder
        merry_requestHdlr_panic(_PANIC_DECODER_NOT_STARTING); // decoder is not starting
        return RET_NULL;
    }
    _llog_(_CORE_, "ENTERY", "Core ID %lu entering FDE cycle", c->core_id);
    while (mtrue)
    {
        merry_mutex_lock(c->lock);
        if (c->stop_running == mtrue)
        {
            // we have to also tell the decoder to stop
            // if the decoder was behind this error, then it should have already stopped
            _llog_(_CORE_, "STOPPING", "Core ID %lu stopping now", c->core_id);
            merry_cond_signal(c->decoder->cond); // in case the decoder was sleeping
            merry_mutex_lock(c->decoder->lock);
            c->decoder->should_stop = mtrue;
            merry_mutex_unlock(c->decoder->lock);
            break;
        }
        merry_mutex_unlock(c->lock);
        // The only job of the core is:
        // pop one instruction
        // execute it
        // if there was an error in executing the instruction, the function that executes the instruction will
        // perfrom the task of registering the error and making the core exit while also stopping the decoder
        merry_decoder_get_inst(c->decoder);
        _llog_(_CORE_, "EXECUTION", "Core ID %lu executing INST: %lu", c->ir->opcode);
        c->ir->exec_func(c);
        _llog_(_CORE_, "EXECUTED", "Core ID %lu executed INST: %lu", c->ir->opcode);
    }
    _llog_(_CORE_, "TERMINATING", "Core ID %lu terminating execution", c->core_id);
    return RET_NULL; // return nothing
}