#include "ib_singleton.h"

int ib::SingletonFinalizer::count_ = 0;
ib::SingletonFinalizer::FinalizerFunc ib::SingletonFinalizer::funcs_[ib::SingletonFinalizer::MAX_FUNCS_] = {0};

void ib::SingletonFinalizer::addFinalizer(ib::SingletonFinalizer::FinalizerFunc func) {
    funcs_[count_++] = func;
}

void ib::SingletonFinalizer::finalize() {
    for (int i = count_ - 1; i >= 0; --i) {
        (*funcs_[i])();
    }
    count_ = 0;
}
