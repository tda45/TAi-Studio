#include "chatmodel.h"

#include <QDebug>
#include <QMap>
#include <QTextStream>
#include <QtLogging>

#include <exception>


QList<ResultInfo> ChatItem::consolidateSources(const QList<ResultInfo> &sources)
{
    QMap<QString, ResultInfo> groupedData;
    for (const ResultInfo &info : sources) {
        if (groupedData.contains(info.file)) {
            groupedData[info.file].text += "\n---\n" + info.text;
        } else {
            groupedData[info.file] = info;
        }
    }
    QList<ResultInfo> consolidatedSources = groupedData.values();
    return consolidatedSources;
}

void ChatItem::serializeResponse(QDataStream &stream, int version)
{
    stream << value;
}

void ChatItem::serializeToolCall(QDataStream &stream, int version)
{
    stream << value;
    toolCallInfo.serialize(stream, version);
}

void ChatItem::serializeToolResponse(QDataStream &stream, int version)
{
    stream << value;
}

void ChatItem::serializeText(QDataStream &stream, int version)
{
    stream << value;
}

void ChatItem::serializeThink(QDataStream &stream, int version)
{
    stream << value;
    stream << thinkingTime;
}

void ChatItem::serializeSubItems(QDataStream &stream, int version)
{
    stream << name;
    switch (auto typ = type()) {
        using enum ChatItem::Type;
        case Response:       { serializeResponse(stream, version);      break; }
        case ToolCall:      { serializeToolCall(stream, version);      break; }
        case ToolResponse:  { serializeToolResponse(stream, version);   break; }
        case Text:          { serializeText(stream, version);           break; }
        case Think:         { serializeThink(stream, version);          break; }
        case System:
        case Prompt:
            throw std::invalid_argument(fmt::format("alt öge tipi serileştirilemiyor: {}", int(typ)));
    }

    stream << qsizetype(subItems.size());
    for (ChatItem *item : subItems)
        item->serializeSubItems(stream, version);
}

void ChatItem::serialize(QDataStream &stream, int version)
{
    stream << name;
    stream << value;
    stream << newResponse;
    stream << isCurrentResponse;
    stream << stopped;
    stream << thumbsUpState;
    stream << thumbsDownState;
    if (version >= 11 && type() == ChatItem::Type::Response)
        stream << isError;
    if (version >= 8) {
        stream << sources.size();
        for (const ResultInfo &info : sources) {
            Q_ASSERT(!info.file.isEmpty());
            stream << info.collection;
            stream << info.path;
            stream << info.file;
            stream << info.title;
            stream << info.author;
            stream << info.date;
            stream << info.text;
            stream << info.page;
            stream << info.from;
            stream << info.to;
        }
    } else if (version >= 3) {
        QList<QString> references;
        QList<QString> referencesContext;
        int validReferenceNumber = 1;
        for (const ResultInfo &info : sources) {
            if (info.file.isEmpty())
                continue;

            QString reference;
            {
                QTextStream stream(&reference);
                stream << (validReferenceNumber++) << ". ";
                if (!info.title.isEmpty())
                    stream << "\"" << info.title << "\". ";
                if (!info.author.isEmpty())
                    stream << "Yazar: " << info.author << ". ";
                if (!info.date.isEmpty())
                    stream << "Tarih: " << info.date << ". ";
                stream << "Dosya: " << info.file << ". ";
                if (info.page != -1)
                    stream << "Sayfa " << info.page << ". ";
                if (info.from != -1) {
                    stream << "Satır " << info.from;
                    if (info.to != -1)
                        stream << "-" << info.to;
                    stream << ". ";
                }
                stream << "[Bağlam](context://" << validReferenceNumber - 1 << ")";
            }
            references.append(reference);
            referencesContext.append(info.text);
        }

        stream << references.join("\n");
        stream << referencesContext;
    }
    if (version >= 10) {
        stream << promptAttachments.size();
        for (const PromptAttachment &a : promptAttachments) {
            Q_ASSERT(!a.url.isEmpty());
            stream << a.url;
            stream << a.content;
        }
    }

    if (version >= 12) {
        stream << qsizetype(subItems.size());
        for (ChatItem *item : subItems)
            item->serializeSubItems(stream, version);
    }
}

bool ChatItem::deserializeToolCall(QDataStream &stream, int version)
{
    stream >> value;
    return toolCallInfo.deserialize(stream, version);
}

bool ChatItem::deserializeToolResponse(QDataStream &stream, int version)
{
    stream >> value;
    return true;
}

bool ChatItem::deserializeText(QDataStream &stream, int version)
{
    stream >> value;
    return true;
}

bool ChatItem::deserializeResponse(QDataStream &stream, int version)
{
    stream >> value;
    return true;
}

bool ChatItem::deserializeThink(QDataStream &stream, int version)
{
    stream >> value;
    stream >> thinkingTime;
    return true;
}

bool ChatItem::deserializeSubItems(QDataStream &stream, int version)
{
    stream >> name;
    try {
        type(); // ismi kontrol et
    } catch (const std::exception &e) {
        qWarning() << "ChatModel HATASI:" << e.what();
        return false;
    }
    switch (auto typ = type()) {
        using enum ChatItem::Type;
        case Response:      { deserializeResponse(stream, version); break; }
        case ToolCall:      { deserializeToolCall(stream, version); break; }
        case ToolResponse:  { deserializeToolResponse(stream, version); break; }
        case Text:          { deserializeText(stream, version); break; }
        case Think:         { deserializeThink(stream, version); break; }
        case System:
        case Prompt:
            throw std::invalid_argument(fmt::format("alt öge tipi serileştirilemiyor: {}", int(typ)));
    }

    qsizetype count;
    stream >> count;
    for (int i = 0; i < count; ++i) {
        ChatItem *c = new ChatItem(this);
        if (!c->deserializeSubItems(stream, version)) {
            delete c;
            return false;
        }
        subItems.push_back(c);
    }

    return true;
}

bool ChatItem::deserialize(QDataStream &stream, int version)
{
    if (version < 12) {
        int id;
        stream >> id;
    }
    stream >> name;
    try {
        type(); // ismi kontrol et
    } catch (const std::exception &e) {
        qWarning() << "ChatModel HATASI:" << e.what();
        return false;
    }
    stream >> value;
    if (version < 10) {
        // Bu özellik kullanımdan kaldırıldı ve artık kullanılmıyor
        QString prompt;
        stream >> prompt;
    }
    stream >> newResponse;
    stream >> isCurrentResponse;
    stream >> stopped;
    stream >> thumbsUpState;
    stream >> thumbsDownState;
    if (version >= 11 && type() == ChatItem::Type::Response)
        stream >> isError;
    if (version >= 8) {
        qsizetype count;
        stream >> count;
        for (int i = 0; i < count; ++i) {
            ResultInfo info;
            stream >> info.collection;
            stream >> info.path;
            stream >> info.file;
            stream >> info.title;
            stream >> info.author;
            stream >> info.date;
            stream >> info.text;
            stream >> info.page;
            stream >> info.from;
            stream >> info.to;
            sources.append(info);
        }
        consolidatedSources = ChatItem::consolidateSources(sources);
    } else if (version >= 3) {
        QString references;
        QList<QString> referencesContext;
        stream >> references;
        stream >> referencesContext;

        if (!references.isEmpty()) {
            QList<QString> referenceList = references.split("\n");

            // Boş satırları ve artık kullanılmayan "---" ile başlayan satırları yoksay
            for (auto it = referenceList.begin(); it != referenceList.end();) {
                if (it->trimmed().isEmpty() || it->trimmed().startsWith("---"))
                    it = referenceList.erase(it);
                else
                    ++it;
            }

            Q_ASSERT(referenceList.size() == referencesContext.size());
            for (int j = 0; j < referenceList.size(); ++j) {
                QString reference = referenceList[j];
                QString context = referencesContext[j];
                ResultInfo info;
                QTextStream refStream(&reference);
                QString dummy;
                int validReferenceNumber;
                refStream >> validReferenceNumber >> dummy;
                
                // Başlığı ayıkla (tırnak işaretleri arasındakiler)
                if (reference.contains("\"")) {
                    int startIndex = reference.indexOf('"') + 1;
                    int endIndex = reference.indexOf('"', startIndex);
                    info.title = reference.mid(startIndex, endIndex - startIndex);
                }

                // Yazarı ayıkla ("By " sonrasını ve bir sonraki noktadan öncesini alır)
                // Not: Eski serileştirilmiş verileri okuyabilmek için "By " metni korunmuştur.
                if (reference.contains("By ")) {
                    int startIndex = reference.indexOf("By ") + 3;
                    int endIndex = reference.indexOf('.', startIndex);
                    info.author = reference.mid(startIndex, endIndex - startIndex).trimmed();
                }

                // Tarihi ayıkla ("Date: " sonrasını ve bir sonraki noktadan öncesini alır)
                if (reference.contains("Date: ")) {
                    int startIndex = reference.indexOf("Date: ") + 6;
                    int endIndex = reference.indexOf('.', startIndex);
                    info.date = reference.mid(startIndex, endIndex - startIndex).trimmed();
                }

                // Dosya adını ayıkla ("In " sonrası ve "[Context]" öncesi)
                if (reference.contains("In ") && reference.contains(". [Context]")) {
                    int startIndex = reference.indexOf("In ") + 3;
                    int endIndex = reference.indexOf(". [Context]", startIndex);
                    info.file = reference.mid(startIndex, endIndex - startIndex).trimmed();
                }

                // Sayfa numarasını ayıkla ("Page " sonrası ve bir sonraki boşluk öncesi)
                if (reference.contains("Page ")) {
                    int startIndex = reference.indexOf("Page ") + 5;
                    int endIndex = reference.indexOf(' ', startIndex);
                    if (endIndex == -1) endIndex = reference.length();
                    info.page = reference.mid(startIndex, endIndex - startIndex).toInt();
                }

                // Satırları ayıkla ("Lines " sonrası ve sonraki boşluk ya da tire işareti öncesi)
                if (reference.contains("Lines ")) {
                    int startIndex = reference.indexOf("Lines ") + 6;
                    int endIndex = reference.indexOf(' ', startIndex);
                    if (endIndex == -1) endIndex = reference.length();
                    int hyphenIndex = reference.indexOf('-', startIndex);
                    if (hyphenIndex != -1 && hyphenIndex < endIndex) {
                        info.from = reference.mid(startIndex, hyphenIndex - startIndex).toInt();
                        info.to = reference.mid(hyphenIndex + 1, endIndex - hyphenIndex - 1).toInt();
                    } else {
                        info.from = reference.mid(startIndex, endIndex - startIndex).toInt();
                    }
                }
                info.text = context;
                sources.append(info);
            }

            consolidatedSources = ChatItem::consolidateSources(sources);
        }
    }
    if (version >= 10) {
        qsizetype count;
        stream >> count;
        QList<PromptAttachment> attachments;
        for (int i = 0; i < count; ++i) {
            PromptAttachment a;
            stream >> a.url;
            stream >> a.content;
            attachments.append(a);
        }
        promptAttachments = attachments;
    }

    if (version >= 12) {
        qsizetype count;
        stream >> count;
        for (int i = 0; i < count; ++i) {
            ChatItem *c = new ChatItem(this);
            if (!c->deserializeSubItems(stream, version)) {
                delete c;
                return false;
            }
            subItems.push_back(c);
        }
    }
    return true;
}