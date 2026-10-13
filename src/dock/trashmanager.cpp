#include "trashmanager.h"

#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QUrl>

#include <QStandardPaths>

static QString trashDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/Trash");
}

static const QDir::Filters ItemsShouldCount = QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot;

TrashManager::TrashManager(QObject *parent)
    : QObject(parent),
      m_filesWatcher(new QFileSystemWatcher(this)),
      m_count(0)
{
    const QString base = trashDir();
    m_filesWatcher->addPath(base);
    const QString files = base + QLatin1String("/files");
    if (QDir(files).exists()) {
        m_filesWatcher->addPath(files);
    }

    onDirectoryChanged();
    connect(m_filesWatcher, &QFileSystemWatcher::directoryChanged, this, &TrashManager::onDirectoryChanged, Qt::QueuedConnection);
}

void TrashManager::moveToTrash(const QList<QUrl> &urls)
{
    QStringList paths;
    paths.reserve(urls.size());

    for (const QUrl &url : urls) {
        if (!url.isLocalFile())
            continue;
        paths.append(url.toLocalFile());
    }

    QProcess::startDetached(QStringLiteral("cutefish-filemanager"), QStringList() << QStringLiteral("--move-to-trash") << paths);
}

void TrashManager::emptyTrash()
{
    QProcess::startDetached(QStringLiteral("cutefish-filemanager"), QStringList() << QStringLiteral("-e"));
}

void TrashManager::openTrash()
{
    QProcess::startDetached(QStringLiteral("cutefish-filemanager"), QStringList() << QStringLiteral("trash:///"));
}

void TrashManager::onDirectoryChanged()
{
    const QString filesPath = trashDir() + QLatin1String("/files");
    const QDir files(filesPath);

    if (files.exists()) {
        if (!m_filesWatcher->directories().contains(filesPath)) {
            m_filesWatcher->addPath(filesPath);
        }
        m_count = files.entryList(ItemsShouldCount).count();
    } else {
        m_count = 0;
    }

    emit countChanged();
}
