/***************************************************************************
 AQSObject_p.h
 -------------------
 begin                : 29/01/2011
 copyright            : (C) 2003-2011 by InfoSiAL S.L.
 email                : mail@infosial.com
 ***************************************************************************/
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 ***************************************************************************/
/***************************************************************************
 Este  programa es software libre. Puede redistribuirlo y/o modificarlo
 bajo  los  t�rminos  de  la  Licencia  P�blica General de GNU   en  su
 versi�n 2, publicada  por  la  Free  Software Foundation.
 ***************************************************************************/

#ifndef AQSOBJECT_P_H_
#define AQSOBJECT_P_H_

#include "AQS_p.h"
#include "AQSEvent_p.h"
#include <qscrollview.h>
#include <qbitarray.h>
#include <qguardedptr.h>

void __aq_baseclass_init_error( 
    const char * prefix, const char * clname, const char * bclname, 
    QObject *qo );

#define AQ_DECLARE_XYZ_PREFIX_OBJECT_PTYPE(CXYZ, PXYZ, Prefix,Class,BaseClass, PtrType) \
  protected: \
  void internalInit(PtrType *o) { \
    o_ = o; \
    PXYZ##BaseClass::internalInit(o); \
  } \
  private: \
  /*PtrType *o_;*/ \
  QGuardedPtr<PtrType> o_;\
  void init(QObject *qo) { \
    o_ = ::qt_cast<PtrType *>(qo); \
    if (!o_) { \
      __aq_baseclass_init_error(AQ_QUOTEME(Prefix), AQ_QUOTEME(Class), AQ_QUOTEME(BaseClass), qo ); \
    } else { \
      QObject::setName(AQ_QUOTEME(PtrType)); \
      PXYZ##BaseClass::internalInit(o_); \
      AQS_IF_DEBUG(printf("%s init %s %p\n", AQ_QUOTEME(CXYZ##Class), o_->QObject::name(), (PtrType *)o_)); \
      specializedInternalInit(); \
    } \
  }\
  void finish() { \
    if (!finished_) { \
      AQS_IF_DEBUG(printf("%s finish %p\n", AQ_QUOTEME(CXYZ##Class), (PtrType *)o_)); \
      finished_ = true; \
      specializedInternalFinish(); \
    } \
  }\
  public: \
  virtual const char *RTTI() const { return #Class; } \
  operator PtrType *() { return o_; } \
  operator const PtrType *() const { return o_; } \
  operator PtrType &() { if (o_) return *o_; } \
  operator const PtrType &() const { if (o_) return *o_; } \
  CXYZ##Class() : PXYZ##BaseClass (), o_(0) {} \
  CXYZ##Class(QObject *qo) : PXYZ##BaseClass () {init(qo);} \
  virtual ~CXYZ##Class() { \
    finish(); \
  } \
  AQ_STATIC_CONSTRUCTOR_PTYPE(Prefix,Class, PtrType)

#define AQ_DECLARE_XYZ_PREFIX_OBJECT(CXYZ, PXYZ, Prefix,Class,BaseClass) AQ_DECLARE_XYZ_PREFIX_OBJECT_PTYPE(CXYZ, PXYZ, Prefix,Class,BaseClass, Prefix##Class)

#define AQ_DECLARE_AQS_PREFIX_OBJECT(Prefix,Class,BaseClass)  AQ_DECLARE_XYZ_PREFIX_OBJECT(AQS, AQS, Prefix,Class,BaseClass)

#define AQ_DECLARE_AQS_OBJECT(Class,BaseClass) AQ_DECLARE_AQS_PREFIX_OBJECT(Q,Class,BaseClass)
#define AQ_DECLARE_AQS_AQOBJECT(Class,BaseClass) AQ_DECLARE_AQS_PREFIX_OBJECT(AQ,Class,BaseClass)
#define AQ_DECLARE_AQS_FLOBJECT(Class,BaseClass) AQ_DECLARE_XYZ_PREFIX_OBJECT(FLS, AQS, FL,Class,BaseClass)
#define AQ_DECLARE_AQS_FLOBJECT_PTYPE(Class,BaseClass, PtrType) AQ_DECLARE_XYZ_PREFIX_OBJECT_PTYPE(FLS, AQS, FL,Class,BaseClass, PtrType)
#define AQ_DECLARE_AQS_AQFLOBJECT(Class,BaseClass) AQ_DECLARE_XYZ_PREFIX_OBJECT(AQS, FLS, AQ,Class,BaseClass)

class AQSObject : public AQSBaseObject
{
  Q_OBJECT

  Q_PROPERTY(QString eventFilterFunction READ eventFilterFunction WRITE setEventFilterFunction)
  Q_PROPERTY(QValueList allowedEvents READ allowedEvents WRITE setAllowedEvents)

  Q_OVERRIDE(QCString name READ objectName WRITE setObjectName)

  AQ_DECLARE_AQS(Object)

  AQ_STATIC_CONSTRUCTOR(Q, Object)

public:
  AQSObject(QObject *qo) : AQSBaseObject() {
    init(qo);
  }

  virtual bool eventFilter(QObject *o, QEvent *e) {
    if (!filterArgs_ || e->type() > 99 || !bitAllowedEvents_.testBit(e->type()))
      return false;
    filterEvent_->setEvent(e);
    filterArgs_->first() = o;
    return globalAQSInterpreter->call(eventFilterFunction_, *filterArgs_, 0).variant().toBool();
  }

  void setEventFilterFunction(const QString &functionName) {
    if (filterArgs_ == 0) {
      filterEvent_ = new AQSEvent;
      filterArgs_ = new QSArgumentList;
      filterArgs_->append(QSArgument());
      filterArgs_->append(filterEvent_);
    }
    eventFilterFunction_ = functionName;
  }

  QString eventFilterFunction() const {
    return eventFilterFunction_;
  }

  void setAllowedEvents(const QValueList<QVariant> &allowed) {
    bitAllowedEvents_.fill(false, 100);
    for (QValueList<QVariant>::const_iterator it = allowed.begin();
         it != allowed.end(); ++it) {
      bitAllowedEvents_.setBit((*it).toInt());
    }
    allowedEvents_ = allowed;
  }

  QValueList<QVariant> allowedEvents() const {
    return allowedEvents_;
  }

  void setObjectName(const QCString &name) {
    AQ_CALL_VOID(QObject::setName(name));
  }

  QCString objectName() const {
    AQ_CALL_RET(QObject::name());
  }

public slots :
  bool isEqual(QObject *other) const {
    return (o_ ? o_ == other : false);
  }

  bool isEqual(AQSBaseObject *other) const {
    return ((o_ && other) ? o_ == other->o() : false);
  }

  QObject *parent() const {
    AQ_CALL_RET(parent());
  }

  QObject *child(const QString &objName,
                 const QString &inheritsClass = QString::null,
                 bool recursiveSearch = true) {
    AQ_CALL_RET(child(objName.latin1(), inheritsClass.latin1(),
                      recursiveSearch));
  }

  void installEventFilter(QObject *filterObj) {
    AQSBaseObject *aqo = globalAQS->findAQSObject(filterObj);
    AQ_CALL_VOID(installEventFilter(aqo));

    QScrollView *sco = ::qt_cast<QScrollView *>(o_);
    if (sco) {
      QWidget *v = sco->viewport();
      v->QObject::setName(o_->QObject::name());
      v->installEventFilter(aqo);
    }
  }

  void removeEventFilter(QObject *obj) {
    AQ_CALL_VOID(removeEventFilter(globalAQS->findAQSObject(obj)));
  }

  bool isWidgetType() const {
    AQ_CALL_RET_V(isWidgetType(), bool);
  }

  bool isA(const QString &clname) const {
    AQ_CALL_RET_V(isA(clname), bool);
  }

  bool inherits(const QString &clname) const {
    AQ_CALL_RET_V(inherits(clname), bool);
  }

  int startInternalTimer(int interval) {
    AQ_CALL_RET_V(startTimer(interval), int);
  }

  void killInternalTimer(int id) {
    AQ_CALL_VOID(killTimer(id));
  }

  virtual QCString className() {
    AQ_CALL_RET(className());
  }

  virtual QCString rtti() {
    return RTTI();
  }

  virtual void insertChild(QObject *obj) {
    AQ_CALL_VOID(insertChild(obj));
  }

  virtual void removeChild(QObject *obj) {
    AQ_CALL_VOID(removeChild(obj));
  }

  virtual QDomNode *toXml(bool includeComplexTypes = false) {
    xml_.clear();
    QDomElement e(xml_.createElement("object"));

    e.setAttribute("class", RTTI());
    xml_.appendChild(e);

    if (!o_)
      return &xml_;

    QMap<QString, bool> propMap;
    QMap<QCString, bool> override;
    const QMetaObject *m = o_->metaObject();
    int num = m->numProperties(true);

    for (int j = 0; j < num; ++j) {
      const QMetaProperty *mp = m->property(j, true);
      if (mp->testFlags(QMetaProperty::Override))
        override[mp->name()] = true;
      else if (override.contains(mp->name()))
        continue;

      if (propMap.find(mp->name()) != propMap.end())
        continue;
      propMap.replace(mp->name(), false);

      QVariant val(o_->property(mp->name()));
      if (val.isNull() || !val.isValid())
        continue;

      if (!val.canCast(QVariant::String)) {
        if (!includeComplexTypes)
          continue;
        QByteArray ba;
        QDataStream ds(ba, IO_WriteOnly);
        val.save(ds);
        val = QVariant(byteArrayToBase64(&ba));
      }

      QString valStr(val.toString());
      if (!valStr.isEmpty())
        e.setAttribute(mp->name(), valStr);
    }

    return &xml_;
  }

protected:
  void internalInit(void *o) {
    o_ = 0;
    AQSBaseObject::internalInit(o);
  }

  void internalInit(QObject *o) {
    o_ = o;
    AQSBaseObject::internalInit(o);
  }

  static void *construct(const QSArgumentList &args) {
    QMap<int, QStringList> candidates;
    candidates[0].append(QString::null);
    candidates[1].append(QString::fromLatin1("QObject*"));
    candidates[1].append(QString::fromLatin1("QString"));
    candidates[2].append(QString::fromLatin1("QObject*,QString"));
    QString sgt(castArgsSignature(args, candidates));
    if (sgt.isEmpty())
      return new QObject;
    if (sgt == QString::fromLatin1("QObject*"))
      return new QObject(argValue<QObject *>(args[0]));
    if (sgt == QString::fromLatin1("QString"))
      return new QObject(0, *(argValue<QString *>(args[0])));
    if (sgt == QString::fromLatin1("QObject*,QString"))
      return new QObject(argValue<QObject *>(args[0]),
                         *(argValue<QString *>(args[1])));
    return 0;
  }

public:
  static QMap<int, QStringList> candidateConstructors() {
    QMap<int, QStringList> candidates;
    candidates[0].append(QString::null);
    candidates[1].append(QString::fromLatin1("QObject*"));
    candidates[1].append(QString::fromLatin1("QString"));
    candidates[2].append(QString::fromLatin1("QObject*,QString"));
    return candidates;
  }

private:
  QObject *o_;
  QString eventFilterFunction_;
  QVariantList allowedEvents_;
  QBitArray bitAllowedEvents_;
  static AQSEvent *filterEvent_;
  static QSArgumentList *filterArgs_;
};

#endif /* AQSOBJECT_P_H_ */
// @AQSOBJECT@
