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
        QStringLiteral("org.hnjae.kiriview.decode"),
        QStringLiteral("org.hnjae.kiriview.navigation"),
        QStringLiteral("org.hnjae.kiriview.predecode"),
        QStringLiteral("org.hnjae.kiriview.thumbnail"),
        QStringLiteral("org.hnjae.kiriview.display.provider"),
        QStringLiteral("org.hnjae.kiriview.animation"),
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
