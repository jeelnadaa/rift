// Typed settings store (PLAN 8). Values live in an INI file in the data folder and are exposed to
// QML as a QQmlPropertyMap, so bindings like `App.settings.theme` update live.
#pragma once
#include <QQmlPropertyMap>
#include <QSettings>
#include <QVariantMap>

class Settings : public QQmlPropertyMap {
    Q_OBJECT
public:
    explicit Settings(const QString &iniPath, QObject *parent = nullptr);

    static QVariantMap defaults();
    QVariant get(const QString &key) const { return value(key); }
    int i(const QString &key) const { return value(key).toInt(); }
    bool b(const QString &key) const { return value(key).toBool(); }
    QString s(const QString &key) const { return value(key).toString(); }
    double d(const QString &key) const { return value(key).toDouble(); }

    Q_INVOKABLE void set(const QString &key, const QVariant &v);
    Q_INVOKABLE void reset(const QString &key);
    Q_INVOKABLE void resetAll();
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString &json);

signals:
    void changed(const QString &key);

protected:
    QVariant updateValue(const QString &key, const QVariant &input) override;

private:
    QSettings m_store;
};
