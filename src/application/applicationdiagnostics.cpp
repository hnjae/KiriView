// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationdiagnostics.h"

#include <QLoggingCategory>
#include <QString>
#include <QStringList>

namespace {
QStringList verboseDiagnosticCategoryNames()
{
    return {
        QStringLiteral("io.github.hnjae.kiriview.decode"),
        QStringLiteral("io.github.hnjae.kiriview.navigation"),
        QStringLiteral("io.github.hnjae.kiriview.predecode"),
        QStringLiteral("io.github.hnjae.kiriview.thumbnail"),
        QStringLiteral("io.github.hnjae.kiriview.display.provider"),
        QStringLiteral("io.github.hnjae.kiriview.animation"),
    };
}

QString verboseDiagnosticFilterRules()
{
    QStringList rules;
    for (const QString &categoryName : verboseDiagnosticCategoryNames()) {
        rules.append(QStringLiteral("%1.debug=true").arg(categoryName));
    }
    return rules.join(QLatin1Char('\n'));
}
}

namespace KiriView {
void configureApplicationDiagnosticLogging(bool verbose)
{
    if (!verbose) {
        return;
    }

    QLoggingCategory::setFilterRules(verboseDiagnosticFilterRules());
}
}
