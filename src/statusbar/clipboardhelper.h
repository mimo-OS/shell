#ifndef CLIPBOARDHELPER_H
#define CLIPBOARDHELPER_H

#include <QObject>
#include <QStringList>
#include <QClipboard>
#include <QGuiApplication>

class ClipboardHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int count READ count NOTIFY historyChanged)

public:
    explicit ClipboardHelper(QObject *parent = nullptr);

    QStringList history() const;
    int count() const;

    Q_INVOKABLE void copy(const QString &text);
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void clear();

signals:
    void historyChanged();
    void itemCopied(const QString &text);

private slots:
    void onClipboardChanged();

private:
    QStringList m_history;
    bool m_internalCopy = false;
};

#endif // CLIPBOARDHELPER_H
