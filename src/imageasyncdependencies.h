// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCDEPENDENCIES_H
#define KIRIVIEW_IMAGEASYNCDEPENDENCIES_H

#include "archive/archivebackend.h"
#include "document/filedeletion.h"
#include "imagedecodedependencies.h"
#include "navigation/imagecandidateprovider.h"
#include "powersaverprovider.h"

namespace KiriView {
struct ImageAsyncDependencies {
    ImageNavigationCandidateProvider candidateProvider;
    ImageDecodeDependencies imageDecode;
    FileOperationProvider fileOperations;
    ArchiveDocumentSessionFactory archiveDocumentSessions;
    PowerSaverProvider powerSaver;
};
}

#endif
