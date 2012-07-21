/*
 * Copyright (C) 2008-2012 The QXmpp developers
 *
 * Author:
 *  Manjeet Dahiya
 *
 * Source:
 *  http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */


#include "QXmppPresence.h"
#include "QXmppUtils.h"
#include <QtDebug>
#include <QDomElement>
#include <QXmlStreamWriter>
#include "QXmppConstants.h"

static const char* presence_types[] = {
    "error",
    "",
    "unavailable",
    "subscribe",
    "subscribed",
    "unsubscribe",
    "unsubscribed",
    "probe"
};

static const char* presence_shows[] = {
    "",
    "away",
    "xa",
    "dnd",
    "chat",
    "invisible"
};

/// Constructs a QXmppPresence.
///
/// \param type

QXmppPresence::QXmppPresence(QXmppPresence::Type type)
    : m_type(type),
    m_vCardUpdateType(VCardUpdateNone)
{
}

QXmppPresence::QXmppPresence(QXmppPresence::Type type,
                             const QXmppPresence::Status& status)
    : QXmppStanza(),
    m_type(type),
    m_status(status),
    m_vCardUpdateType(VCardUpdateNone)
{
}

/// Destroys a QXmppPresence.

QXmppPresence::~QXmppPresence()
{

}

/// Returns the availability status type, for instance busy or away.
///
/// This will not tell you whether a contact is connected, check whether
/// type() is QXmppPresence::Available instead.

QXmppPresence::AvailableStatusType QXmppPresence::availableStatusType() const
{
    return static_cast<QXmppPresence::AvailableStatusType>(m_status.type());
}

/// Sets the availability status type, for instance busy or away.

void QXmppPresence::setAvailableStatusType(AvailableStatusType type)
{
    m_status.setType(static_cast<QXmppPresence::Status::Type>(type));
}

/// Returns the priority level of the resource.

int QXmppPresence::priority() const
{
    return m_status.priority();
}

/// Sets the \a priority level of the resource.

void QXmppPresence::setPriority(int priority)
{
    m_status.setPriority(priority);
}

/// Returns the status text, a textual description of the user's status.

QString QXmppPresence::statusText() const
{
    return m_status.statusText();
}

/// Sets the status text, a textual description of the user's status.
///
/// \param statusText The status text, for example "Gone fishing".

void QXmppPresence::setStatusText(const QString& statusText)
{
    m_status.setStatusText(statusText);
}

/// Returns the presence type.
///
/// You can use this method to determine the action which needs to be
/// taken in response to receiving the presence. For instance, if the type is
/// QXmppPresence::Available or QXmppPresence::Unavailable, you could update
/// the icon representing a contact's availability.

QXmppPresence::Type QXmppPresence::type() const
{
    return m_type;
}

/// Sets the presence type.
///
/// \param type

void QXmppPresence::setType(QXmppPresence::Type type)
{
    m_type = type;
}

/// \cond
void QXmppPresence::parse(const QDomElement &element)
{
    QXmppStanza::parse(element);

    const QString type = element.attribute("type");
    for (int i = Error; i <= Probe; i++) {
        if (type == presence_types[i]) {
            m_type = static_cast<Type>(i);
            break;
        }
    }
    m_status.parse(element);

    QXmppElementList extensions;
    QDomElement xElement = element.firstChildElement();
    m_vCardUpdateType = VCardUpdateNone;
    while(!xElement.isNull())
    {
        // XEP-0045: Multi-User Chat
        if(xElement.namespaceURI() == ns_muc_user)
        {
            QDomElement itemElement = xElement.firstChildElement("item");
            m_mucItem.parse(itemElement);
            QDomElement statusElement = xElement.firstChildElement("status");
            m_mucStatusCodes.clear();
            while (!statusElement.isNull()) {
                m_mucStatusCodes << statusElement.attribute("code").toInt();
                statusElement = statusElement.nextSiblingElement("status");
            }
        }
        // XEP-0153: vCard-Based Avatars
        else if(xElement.namespaceURI() == ns_vcard_update)
        {
            QDomElement photoElement = xElement.firstChildElement("photo");
            if(!photoElement.isNull())
            {
                m_photoHash = QByteArray::fromHex(photoElement.text().toAscii());
                if(m_photoHash.isEmpty())
                    m_vCardUpdateType = VCardUpdateNoPhoto;
                else
                    m_vCardUpdateType = VCardUpdateValidPhoto;
            }
            else
            {
                m_photoHash = QByteArray();
                m_vCardUpdateType = VCardUpdateNotReady;
            }
        }
        // XEP-0115: Entity Capabilities
        else if(xElement.tagName() == "c" && xElement.namespaceURI() == ns_capabilities)
        {
            m_capabilityNode = xElement.attribute("node");
            m_capabilityVer = QByteArray::fromBase64(xElement.attribute("ver").toAscii());
            m_capabilityHash = xElement.attribute("hash");
            m_capabilityExt = xElement.attribute("ext").split(" ", QString::SkipEmptyParts);
        }
        else if (xElement.tagName() == "error")
        {
        }
        else if (xElement.tagName() == "show")
        {
        }
        else if (xElement.tagName() == "status")
        {
        }
        else if (xElement.tagName() == "priority")
        {
        }
        else
        {
            // other extensions
            extensions << QXmppElement(xElement);
        }
        xElement = xElement.nextSiblingElement();
    }
    setExtensions(extensions);
}

void QXmppPresence::toXml(QXmlStreamWriter *xmlWriter) const
{
    xmlWriter->writeStartElement("presence");
    helperToXmlAddAttribute(xmlWriter,"xml:lang", lang());
    helperToXmlAddAttribute(xmlWriter,"id", id());
    helperToXmlAddAttribute(xmlWriter,"to", to());
    helperToXmlAddAttribute(xmlWriter,"from", from());
    helperToXmlAddAttribute(xmlWriter,"type", presence_types[m_type]);
    m_status.toXml(xmlWriter);

    error().toXml(xmlWriter);

    // XEP-0045: Multi-User Chat
    if(!m_mucItem.isNull() || !m_mucStatusCodes.isEmpty())
    {
        xmlWriter->writeStartElement("x");
        xmlWriter->writeAttribute("xmlns", ns_muc_user);
        if (!m_mucItem.isNull())
            m_mucItem.toXml(xmlWriter);
        foreach (int code, m_mucStatusCodes) {
            xmlWriter->writeStartElement("status");
            xmlWriter->writeAttribute("code", QString::number(code));
            xmlWriter->writeEndElement();
        }
        xmlWriter->writeEndElement();
    }

    // XEP-0153: vCard-Based Avatars
    if(m_vCardUpdateType != VCardUpdateNone)
    {
        xmlWriter->writeStartElement("x");
        xmlWriter->writeAttribute("xmlns", ns_vcard_update);
        switch(m_vCardUpdateType)
        {
        case VCardUpdateNoPhoto:
            helperToXmlAddTextElement(xmlWriter, "photo", "");
            break;
        case VCardUpdateValidPhoto:
            helperToXmlAddTextElement(xmlWriter, "photo", m_photoHash.toHex());
            break;
        case VCardUpdateNotReady:
            break;
        default:
            break;
        }
        xmlWriter->writeEndElement();
    }

    if(!m_capabilityNode.isEmpty() && !m_capabilityVer.isEmpty()
        && !m_capabilityHash.isEmpty())
    {
        xmlWriter->writeStartElement("c");
        xmlWriter->writeAttribute("xmlns", ns_capabilities);
        helperToXmlAddAttribute(xmlWriter, "hash", m_capabilityHash);
        helperToXmlAddAttribute(xmlWriter, "node", m_capabilityNode);
        helperToXmlAddAttribute(xmlWriter, "ver", m_capabilityVer.toBase64());
        xmlWriter->writeEndElement();
    }

    foreach (const QXmppElement &extension, extensions())
        extension.toXml(xmlWriter);

    xmlWriter->writeEndElement();
}
/// \endcond

/// Returns the photo-hash of the VCardUpdate.
///
/// \return QByteArray

QByteArray QXmppPresence::photoHash() const
{
    return m_photoHash;
}

/// Sets the photo-hash of the VCardUpdate.
///
/// \param photoHash as QByteArray

void QXmppPresence::setPhotoHash(const QByteArray& photoHash)
{
    m_photoHash = photoHash;
}

/// Returns the type of VCardUpdate
///
/// \return VCardUpdateType

QXmppPresence::VCardUpdateType QXmppPresence::vCardUpdateType() const
{
    return m_vCardUpdateType;
}

/// Sets the type of VCardUpdate
///
/// \param type VCardUpdateType

void QXmppPresence::setVCardUpdateType(VCardUpdateType type)
{
    m_vCardUpdateType = type;
}

/// XEP-0115: Entity Capabilities
QString QXmppPresence::capabilityHash() const
{
    return m_capabilityHash;
}

/// XEP-0115: Entity Capabilities
void QXmppPresence::setCapabilityHash(const QString& hash)
{
    m_capabilityHash = hash;
}

/// XEP-0115: Entity Capabilities
QString QXmppPresence::capabilityNode() const
{
    return m_capabilityNode;
}

/// XEP-0115: Entity Capabilities
void QXmppPresence::setCapabilityNode(const QString& node)
{
    m_capabilityNode = node;
}

/// XEP-0115: Entity Capabilities
QByteArray QXmppPresence::capabilityVer() const
{
    return m_capabilityVer;
}

/// XEP-0115: Entity Capabilities
void QXmppPresence::setCapabilityVer(const QByteArray& ver)
{
    m_capabilityVer = ver;
}

/// Legacy XEP-0115: Entity Capabilities
QStringList QXmppPresence::capabilityExt() const
{
    return m_capabilityExt;
}

/// Returns the MUC item.

QXmppMucItem QXmppPresence::mucItem() const
{
    return m_mucItem;
}

/// Sets the MUC item.
///
/// \param item

void QXmppPresence::setMucItem(const QXmppMucItem &item)
{
    m_mucItem = item;
}

/// Returns the MUC status codes.

QList<int> QXmppPresence::mucStatusCodes() const
{
    return m_mucStatusCodes;
}

/// Sets the MUC status codes.
///
/// \param codes

void QXmppPresence::setMucStatusCodes(const QList<int> &codes)
{
    m_mucStatusCodes = codes;
}

/// \cond
const QXmppPresence::Status& QXmppPresence::status() const
{
    return m_status;
}

QXmppPresence::Status& QXmppPresence::status()
{
    return m_status;
}

void QXmppPresence::setStatus(const QXmppPresence::Status& status)
{
    m_status = status;
}

QXmppPresence::Status::Status(QXmppPresence::Status::Type type,
                             const QString statusText, int priority) :
                                m_type(type),
                                m_statusText(statusText), m_priority(priority)
{
}

QXmppPresence::Status::Type QXmppPresence::Status::type() const
{
    return m_type;
}

void QXmppPresence::Status::setType(QXmppPresence::Status::Type type)
{
    m_type = type;
}

QString QXmppPresence::Status::statusText() const
{
    return m_statusText;
}

void QXmppPresence::Status::setStatusText(const QString& str)
{
    m_statusText = str;
}

int QXmppPresence::Status::priority() const
{
    return m_priority;
}

void QXmppPresence::Status::setPriority(int priority)
{
    m_priority = priority;
}

void QXmppPresence::Status::parse(const QDomElement &element)
{
    const QString show = element.firstChildElement("show").text();
    for (int i = Online; i <= Invisible; i++) {
        if (show == presence_shows[i]) {
            m_type = static_cast<Type>(i);
            break;
        }
    }
    m_statusText = element.firstChildElement("status").text();
    m_priority = element.firstChildElement("priority").text().toInt();
}

void QXmppPresence::Status::toXml(QXmlStreamWriter *xmlWriter) const
{
    const QString show = presence_shows[m_type];
    if (!show.isEmpty())
        helperToXmlAddTextElement(xmlWriter, "show", show);
    if (!m_statusText.isEmpty())
        helperToXmlAddTextElement(xmlWriter, "status", m_statusText);
    if (m_priority != 0)
        helperToXmlAddTextElement(xmlWriter, "priority", QString::number(m_priority));
}
/// \endcond
