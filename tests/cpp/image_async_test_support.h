// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_IMAGE_ASYNC_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_IMAGE_ASYNC_TEST_SUPPORT_H

#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"
#include "document/filedeletion.h"
#include "system/powersaverprovider.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace KiriView::TestSupport {
namespace Detail {
    template <typename Operation>
    ImageIoJob startManualIoJob(QObject *receiver, const std::shared_ptr<Operation> &operation)
    {
        operation->object = new QObject(receiver);

        std::weak_ptr<Operation> weakOperation = operation;
        ImageIoJob job(operation->object, [weakOperation](QObject *object) {
            if (std::shared_ptr<Operation> operation = weakOperation.lock()) {
                operation->canceled = true;
                operation->object = nullptr;
            }
            if (object != nullptr) {
                object->deleteLater();
            }
        });
        operation->completion = job.completion();
        return job;
    }

    template <typename Operation, typename Delivery>
    void finishManualIoJob(const std::shared_ptr<Operation> &operation, Delivery &&delivery)
    {
        if (operation == nullptr) {
            return;
        }

        QObject *object = operation->object;
        operation->completion.claimAndRun([&]() mutable {
            operation->object = nullptr;
            std::forward<Delivery>(delivery)(*operation);
            if (object != nullptr) {
                object->deleteLater();
            }
        });
    }
}

struct ManualImageDataLoad {
    QObject *object = nullptr;
    QUrl url;
    OpenedCollectionScopeLocation openedCollectionScope;
    ImageFirstDisplayDecodeContext firstDisplay;
    ImageDataCallback dataCallback;
    ErrorCallback errorCallback;
    ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualImageDataLoader
{
public:
    ImageIoJob start(QObject *receiver, ImageDecodeRequest request, ImageDataCallback callback,
        ErrorCallback errorCallback)
    {
        auto load = std::make_shared<ManualImageDataLoad>();
        load->url = request.imageUrl();
        load->openedCollectionScope = request.openedCollectionScope();
        load->firstDisplay = request.firstDisplay();
        load->dataCallback = std::move(callback);
        load->errorCallback = std::move(errorCallback);

        ImageIoJob job = Detail::startManualIoJob(receiver, load);
        m_loads.push_back(load);
        return job;
    }

    std::size_t loadCount() const { return m_loads.size(); }

    bool empty() const { return m_loads.empty(); }

    ManualImageDataLoad &frontLoad() { return *m_loads.front(); }

    const ManualImageDataLoad &frontLoad() const { return *m_loads.front(); }

    ManualImageDataLoad &backLoad() { return *m_loads.back(); }

    const ManualImageDataLoad &backLoad() const { return *m_loads.back(); }

    void finishFrontLoad(QByteArray data) { finishDataLoad(m_loads.front(), std::move(data)); }

    void finishBackLoad(QByteArray data) { finishDataLoad(m_loads.back(), std::move(data)); }

    bool finishOldestActiveLoadForUrl(const QUrl &url, QByteArray data)
    {
        for (const std::shared_ptr<ManualImageDataLoad> &load : m_loads) {
            if (load != nullptr && load->object != nullptr && load->url == url) {
                finishDataLoad(load, std::move(data));
                return true;
            }
        }

        return false;
    }

    void failFrontLoad(const QString &errorString)
    {
        finishLoadError(m_loads.front(), errorString);
    }

    void failBackLoad(const QString &errorString) { finishLoadError(m_loads.back(), errorString); }

    void deliverFrontLoadDataIgnoringCancellation(QByteArray data)
    {
        deliverData(*m_loads.front(), std::move(data));
    }

private:
    template <typename Delivery>
    static void finishLoad(const std::shared_ptr<ManualImageDataLoad> &load, Delivery &&delivery)
    {
        Detail::finishManualIoJob(load, std::forward<Delivery>(delivery));
    }

    static void finishDataLoad(const std::shared_ptr<ManualImageDataLoad> &load, QByteArray data)
    {
        finishLoad(load, [data = std::move(data)](ManualImageDataLoad &load) mutable {
            deliverData(load, std::move(data));
        });
    }

    static void finishLoadError(
        const std::shared_ptr<ManualImageDataLoad> &load, const QString &errorString)
    {
        finishLoad(load, [&errorString](ManualImageDataLoad &load) {
            if (load.errorCallback) {
                load.errorCallback(errorString);
            }
        });
    }

    static void deliverData(ManualImageDataLoad &load, QByteArray data)
    {
        if (load.dataCallback) {
            load.dataCallback(std::move(data));
        }
    }

    std::vector<std::shared_ptr<ManualImageDataLoad>> m_loads;
};

class ManualImageDataLoaderAdapter
{
public:
    explicit ManualImageDataLoaderAdapter(ManualImageDataLoader &dataLoader)
        : m_dataLoader(&dataLoader)
    {
    }

    ImageIoJob operator()(QObject *receiver, ImageDecodeRequest request, ImageDataCallback callback,
        ErrorCallback errorCallback) const
    {
        return m_dataLoader->start(
            receiver, std::move(request), std::move(callback), std::move(errorCallback));
    }

private:
    ManualImageDataLoader *m_dataLoader = nullptr;
};

inline ImageDataLoader dataLoaderFor(ManualImageDataLoader &dataLoader)
{
    return ManualImageDataLoaderAdapter(dataLoader);
}

class ManualPowerSaverMonitor final : public PowerSaverStateMonitor
{
public:
    ManualPowerSaverMonitor(bool enabled, PowerSaverChangedCallback callback)
        : m_enabled(enabled)
        , m_callback(std::move(callback))
    {
    }

    bool powerSaverEnabled() const override { return m_enabled; }

    void setPowerSaverEnabled(bool enabled)
    {
        if (m_enabled == enabled) {
            return;
        }

        m_enabled = enabled;
        if (m_callback) {
            m_callback(enabled);
        }
    }

private:
    bool m_enabled = false;
    PowerSaverChangedCallback m_callback;
};

inline PowerSaverProvider powerSaverProviderFor(
    ManualPowerSaverMonitor *&monitor, bool initialEnabled)
{
    return PowerSaverProvider {
        [&monitor, initialEnabled](QObject *, PowerSaverChangedCallback callback) {
            auto instance
                = std::make_unique<ManualPowerSaverMonitor>(initialEnabled, std::move(callback));
            monitor = instance.get();
            return instance;
        },
    };
}

struct ManualFileOperation {
    QObject *object = nullptr;
    FileDeletionRequest request;
    FileDeletionCallback callback;
    ImageIoJobCompletion completion;
    bool canceled = false;
};

class ManualFileOperationProvider
{
public:
    ImageIoJob start(QObject *receiver, FileDeletionRequest request, FileDeletionCallback callback)
    {
        auto operation = std::make_shared<ManualFileOperation>();
        operation->request = std::move(request);
        operation->callback = std::move(callback);

        ImageIoJob job = Detail::startManualIoJob(receiver, operation);
        m_operations.push_back(operation);
        return job;
    }

    std::size_t operationCount() const { return m_operations.size(); }

    ManualFileOperation &backOperation() { return *m_operations.back(); }

    const ManualFileOperation &backOperation() const { return *m_operations.back(); }

    ManualFileOperation &operationAt(std::size_t index) { return *m_operations.at(index); }

    const ManualFileOperation &operationAt(std::size_t index) const
    {
        return *m_operations.at(index);
    }

    void finishBackOperation(FileDeletionResult result, const QString &errorString = QString())
    {
        finishOperation(m_operations.back(), result, errorString);
    }

    void deliverOperationAtIgnoringCancellation(
        std::size_t index, FileDeletionResult result, const QString &errorString = QString())
    {
        if (m_operations.at(index)->callback) {
            m_operations.at(index)->callback(result, errorString);
        }
    }

private:
    static void finishOperation(const std::shared_ptr<ManualFileOperation> &operation,
        FileDeletionResult result, const QString &errorString)
    {
        Detail::finishManualIoJob(operation, [&](ManualFileOperation &operation) {
            if (operation.callback) {
                operation.callback(result, errorString);
            }
        });
    }

    std::vector<std::shared_ptr<ManualFileOperation>> m_operations;
};

class ManualFileOperationProviderAdapter
{
public:
    explicit ManualFileOperationProviderAdapter(ManualFileOperationProvider &provider)
        : m_provider(&provider)
    {
    }

    ImageIoJob operator()(
        QObject *receiver, FileDeletionRequest request, FileDeletionCallback callback) const
    {
        return m_provider->start(receiver, std::move(request), std::move(callback));
    }

private:
    ManualFileOperationProvider *m_provider = nullptr;
};

inline FileOperationProvider fileOperationProviderFor(ManualFileOperationProvider &provider)
{
    return ManualFileOperationProviderAdapter(provider);
}
}

#endif
