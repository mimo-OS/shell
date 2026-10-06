/*
 * Copyright (C) 2021-2026 CutefishOS Team.
 *
 * Reliable Wayland/X11 Clipboard Helper
 */

#include "clipboardhelper.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>

ClipboardHelper::ClipboardHelper(QObject *parent)
    : QObject(parent)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        connect(clipboard, &QClipboard::dataChanged, this, &ClipboardHelper::onClipboardChanged);

        const QString initial = clipboard->text(QClipboard::Clipboard);
        if (!initial.isEmpty()) {
            m_history.append(initial);
        }
    }
}

QStringList ClipboardHelper::history() const
{
    return m_history;
}

int ClipboardHelper::count() const
{
    return m_history.size();
}

void ClipboardHelper::onClipboardChanged()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return;

    const QString text = clipboard->text(QClipboard::Clipboard);
    if (text.isEmpty())
        return;

    // Avoid duplicate at top of history
    int existing = m_history.indexOf(text);
    if (existing == 0)
        return;

    if (existing > 0) {
        m_history.removeAt(existing);
    }

    m_history.prepend(text);
    while (m_history.size() > 100) {
        m_history.removeLast();
    }

    emit historyChanged();
}

void ClipboardHelper::copy(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        // Move to top of history first
        int idx = m_history.indexOf(text);
        if (idx > 0) {
            m_history.removeAt(idx);
            m_history.prepend(text);
            emit historyChanged();
        } else if (idx == -1) {
            m_history.prepend(text);
            emit historyChanged();
        }

        // Write to system clipboard
        clipboard->setText(text, QClipboard::Clipboard);
        emit itemCopied(text);
    }
}

void ClipboardHelper::remove(int index)
{
    if (index >= 0 && index < m_history.size()) {
        m_history.removeAt(index);
        emit historyChanged();
    }
}

void ClipboardHelper::clear()
{
    if (!m_history.isEmpty()) {
        m_history.clear();
        emit historyChanged();
    }
}
