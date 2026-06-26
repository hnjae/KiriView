// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCOPERATIONSTATE_H
#define KIRIVIEW_IMAGEASYNCOPERATIONSTATE_H

#include "async/imageasyncticket.h"

#include <QtGlobal>
#include <utility>

namespace kiriview {
class ImageAsyncOperationState final
{
public:
    explicit ImageAsyncOperationState(quint64 nextOperationId = 0);

    quint64 start();
    bool accepts(quint64 operationId) const;
    bool finish(quint64 operationId);
    void cancel();
    bool active() const;

private:
    ImageAsyncTicket m_ticket;
    quint64 m_activeOperationId = 0;
};

template <typename Scope> struct ImageAsyncScopedOperation
{
    quint64 operationId = 0;
    Scope scope;
};

template <typename Scope> class ImageAsyncScopedOperationState final
{
public:
    explicit ImageAsyncScopedOperationState(quint64 nextOperationId = 0)
        : m_operation(nextOperationId)
    {
    }

    ImageAsyncScopedOperation<Scope> start(Scope scope)
    {
        m_scope = std::move(scope);
        return ImageAsyncScopedOperation<Scope> {
            m_operation.start(),
            m_scope,
        };
    }

    bool accepts(const ImageAsyncScopedOperation<Scope>& operation) const
    {
        return accepts(operation.operationId, operation.scope);
    }

    bool accepts(quint64 operationId, const Scope& scope) const
    {
        return m_operation.accepts(operationId) && m_scope == scope;
    }

    bool finish(const ImageAsyncScopedOperation<Scope>& operation)
    {
        return finish(operation.operationId, operation.scope);
    }

    bool finish(quint64 operationId, const Scope& scope)
    {
        if (!accepts(operationId, scope)) {
            return false;
        }

        m_scope = Scope();
        return m_operation.finish(operationId);
    }

    void cancel()
    {
        m_scope = Scope();
        m_operation.cancel();
    }

    bool active() const { return m_operation.active(); }

private:
    ImageAsyncOperationState m_operation;
    Scope m_scope;
};
}

#endif
