#include "qleveldb.h"
#include "qleveldbbatch.h"
#include "global.h"
#include <qqmlinfo.h>
#include <qqmlengine.h>
#include <QJsonDocument>
#include <QQmlEngine>

/*!
  \qmltype LevelDB
  \inqmlmodule QtLevelDB
  \brief Low level API to access LevelDB database.

*/
QLevelDB::QLevelDB(QObject *parent)
    : QObject(parent)
    , m_batch(nullptr)
    , m_levelDB(nullptr)
    , m_opened(false)
    , m_status(Status::Undefined)
    , m_statusText("")
    , m_engine(nullptr)
    , m_stringfy()
{

}

QLevelDB::~QLevelDB()
{
    if (m_levelDB)
        delete m_levelDB;
}

void QLevelDB::classBegin()
{
    m_engine = qmlEngine(this);
    if (m_engine){
        m_stringfy = m_engine->globalObject().property("JSON").property("stringify");
    }
}

void QLevelDB::componentComplete()
{
    openDatabase(m_source.toLocalFile());
}

/*!
  \qmlproperty boolean LevelDB::opened
  \readonly
  \brief show when the database is ready to do operations
*/
bool QLevelDB::opened() const
{
    return m_opened;
}

/*!
  \qmlproperty url LevelDB::source
  \brief url that points to the leveldb database folder.

  For now it's only possible to use local file paths.
*/
QUrl QLevelDB::source() const
{
    return m_source;
}


/*!
  \qmlproperty enumeration LevelDB::status
  \brief This property holds information error in case the database is unable to load.
  \table
  \header
    \li {2, 1} \e {LevelDB::Status} is an enumeration:
  \header
    \li Type
    \li Description
  \row
    \li Layer.Undefined (default)
    \li Database is in a unkown state(probably unintialized)
  \row
    \li Layout.Ok
    \li Operation runs ok
  \row
    \li Layout.NotFound
    \li A value is not found for a Key, during get()
  \row
    \li Layout.InvalidArgument
    \li TODO
  \row
    \li Layout.IOError
    \li TODO
  \endtable
*/
QLevelDB::Status QLevelDB::status() const
{
    return m_status;
}

/*!
  \qmlproperty string LevelDB::statusText
  \brief String information about an error when it occurs
*/
QString QLevelDB::statusText() const
{
    return m_statusText;
}

void QLevelDB::setSource(QUrl source)
{
    if(m_source != source){
        m_source = source;
        emit sourceChanged();
        openDatabase(source.toLocalFile());
    }
}

QLevelDBOptions *QLevelDB::options()
{
    leveldb::WriteOptions options;
    return &m_options;
}

/*!
  \qmlmethod QLevelDBBatch* QLevelDB::batch()
  \brief Return an batch item to do batch operations

   The rayCast method is only useful with physics is enabled.
*/
QLevelDBBatch* QLevelDB::batch()
{
    if (m_batch)
        m_batch->deleteLater();
    if (!m_levelDB)
        return nullptr;
    m_batch = new QLevelDBBatch(m_levelDB, this);
    return m_batch;
}

/*!
 * \qmlmethod bool QLevelDB::del(QString key)
*/
bool QLevelDB::del(QString key)
{
    leveldb::WriteOptions options;
    leveldb::Status status = m_levelDB->Delete(options, leveldb::Slice(key.toStdString()));
    return status.ok();
}

bool QLevelDB::put(QString key, QVariant value)
{
    leveldb::WriteOptions options;
    QString json = variantToJson(value);
    if (m_opened && m_levelDB){
        leveldb::Status status = m_levelDB->Put(options,
                       leveldb::Slice(key.toStdString()),
                       leveldb::Slice(json.toStdString()));
        return status.ok();
    }
    return false;
}

bool QLevelDB::putSync(QString key, QVariant value)
{
    leveldb::WriteOptions options;
    QString json = variantToJson(value);
    options.sync = true;
    if (m_opened && m_levelDB){
        leveldb::Status status = m_levelDB->Put(options,
                       leveldb::Slice(key.toStdString()),
                       leveldb::Slice(json.toStdString()));
        return status.ok();
    }
    return false;
}

QVariant QLevelDB::get(QString key)
{
    leveldb::ReadOptions options;
    std::string value = "";
    if (m_opened && m_levelDB){
        leveldb::Status status = m_levelDB->Get(options,
                                                leveldb::Slice(key.toStdString()),
                                                &value);
        if (status.ok())
            return jsonToVariant(QString::fromStdString(value));
    }
    return QVariant();
}

bool QLevelDB::destroyDB(QUrl path)
{
    if (!path.isLocalFile())
        return Status::InvalidArgument;
    if(m_source == path){
        reset();
    }
    leveldb::Options options;
    leveldb::Status status = leveldb::DestroyDB(path.toLocalFile().toStdString(), options);
    return status.ok();
}

bool QLevelDB::repairDB(QUrl path)
{
    if (!path.isLocalFile())
        return Status::InvalidArgument;
    leveldb::Options options;
    leveldb::Status status = leveldb::RepairDB(path.toLocalFile().toStdString(), options);
    return status.ok();
}

void QLevelDB::setStatus(QLevelDB::Status status)
{
    if (m_status != status){
        m_status = status;
        emit statusChanged();
    }
}

void QLevelDB::setStatusText(QString text)
{
    if (m_statusText != text){
        m_statusText = text;
        emit statusTextChanged();
    }
}

void QLevelDB::setOpened(bool opened)
{
    if (m_opened != opened){
        m_opened = opened;
        emit openedChanged();
    }
}

bool QLevelDB::openDatabase(QString localPath)
{
    reset();
    leveldb::Options options = m_options.leveldbOptions();
    leveldb::Status status = leveldb::DB::Open(options,
                                               localPath.toStdString(),
                                               &m_levelDB);

    setOpened(status.ok());
    setStatusText(QString::fromStdString(status.ToString()));
    Status code = parseStatusCode(status);
    setStatus(code);
    return m_opened;
}

void QLevelDB::reset()
{
    if (m_levelDB)
        delete m_levelDB;

    m_levelDB = nullptr;

    setStatus(Status::Undefined);
    setOpened(false);
    setStatusText(QString());
}

QLevelDB::Status QLevelDB::parseStatusCode(leveldb::Status &status)
{
    if (status.ok())
        return Status::Ok;
    if (status.IsCorruption())
        return Status::Corruption;
    if (status.IsIOError())
        return Status::IOError;
    if (status.IsNotFound())
        return Status::NotFound;
    return Status::Undefined;
}

