#include "gvt/core/DatabaseNode.h"
#include "gvt/core/CoreContext.h"
#include "gvt/core/Debug.h"

using namespace gvt::core;

DatabaseNode* DatabaseNode::errNode = new DatabaseNode(String("error"), String("error"), Uuid(nil_uuid()), Uuid(nil_uuid()));

DatabaseNode::DatabaseNode(String name, Variant value, Uuid uuid, Uuid parentUUID)
: p_uuid(uuid), p_name(name), p_value(value), p_parent(parentUUID)
{
}

DatabaseNode::operator bool() const
{
    return (p_uuid != Uuid(nil_uuid())) && (p_parent != Uuid(nil_uuid()));
}

Uuid DatabaseNode::UUID() 
{
    return p_uuid;
}

String DatabaseNode::name() 
{
    return p_name;
}

Uuid DatabaseNode::parentUUID() 
{
    return p_parent;
}

Variant DatabaseNode::value() 
{
    return p_value;
}

void DatabaseNode::setUUID(Uuid uuid) 
{
    p_uuid = uuid;
    //emit uuidChanged();
}

void DatabaseNode::setName(String name) 
{
    p_name = name;
    //emit nameChanged();
}

void DatabaseNode::setParentUUID(Uuid parentUUID) 
{
    p_parent = parentUUID;
    //emit parentUUIDChanged();
}

void DatabaseNode::setValue(Variant value) 
{
    p_value = value;
    //emit valueChanged();
    //propagateUpdate();
}

void DatabaseNode::propagateUpdate()
{
    DatabaseNode* pn;
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
    pn = db.getItem(p_parent);
    Uuid cid = UUID();
    while (pn)
    {
        //emit pn->childChanged(cid);
        cid = pn->UUID();
        pn = db.getItem(pn->parentUUID());
    }
}

Vector<DatabaseNode*> DatabaseNode::getChildren() 
{
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
    return db.getChildren(UUID());
}


/*******************

    DBNodeH

 *******************/


//DBNodeH DBNodeH::errNode();
//DBNodeH* DBNodeH::errNode = new DBNodeH(String("error"), Uuid(0), Uuid(0), 0);

DatabaseNode& DBNodeH::getNode()
{
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
    DatabaseNode* n = db.getItem(_uuid);
    if (n)
        return *n;
    else
     return *DatabaseNode::errNode;
}

DBNodeH DBNodeH::deRef()
{
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
//    DEBUG(value().toUuid().toString().toStdString());
    DatabaseNode& n = getNode();
    DatabaseNode* ref = db.getItem(variant_toUuid(n.value()));
    if (ref && (variant_toUuid(n.value()) != nil_uuid()))
    {
//        DEBUG("success");
        return DBNodeH(ref->UUID());
    }
    else
    {
        GVT_DEBUG(DBG_SEVERE,"DBNodeH deRef failed for uuid " << uuid_toString(_uuid));
        return DBNodeH();
    }
}

DBNodeH DBNodeH::operator[](const String& key)
{
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
    DatabaseNode* child = db.getChildByName(_uuid, key);
    if (!child)
    {
       child = &(ctx->createNode(key).getNode());
   }
   return DBNodeH(child->UUID());
}

DBNodeH& DBNodeH::operator+=(DBNodeH child)
{
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
    child.setParentUUID(UUID());
    db.addChild(UUID(), &(child.getNode()));
    child.propagateUpdate();
    return *this;
}

DBNodeH& DBNodeH::operator=(Variant val)
{
    //DEBUG(p_name.toStdString());
    setValue(val);
    return *this;
}

bool DBNodeH::operator==(const Variant val)
{
   return value() == val;
}

DBNodeH::operator bool() const
{
//    if (this == errNode)
//        DEBUG("is errNode");
//    DEBUG("bool:");
//    DEBUG(p_uuid.toString().toStdString());
    return (_uuid != nil_uuid());
}

Uuid DBNodeH::UUID()
{
    DatabaseNode& n = getNode();
    return n.UUID();
}

String DBNodeH::name() 
{
    DatabaseNode& n = getNode();
    return n.name();
}

Uuid DBNodeH::parentUUID() 
{
    DatabaseNode& n = getNode();
    return n.parentUUID();
}

Variant DBNodeH::value() 
{
    DatabaseNode& n = getNode();
    return n.value();
}


void DBNodeH::setUUID(Uuid uuid) 
{
    _uuid=uuid;
    DatabaseNode& n = getNode();
    n.setUUID(uuid);
}

void DBNodeH::setName(String name) 
{
    DatabaseNode& n = getNode();
    n.setName(name);
}

void DBNodeH::setParentUUID(Uuid parentUUID)  
{
    DatabaseNode& n = getNode();
    n.setParentUUID(parentUUID);
}

void DBNodeH::setValue(Variant value) 
{
    DatabaseNode& n = getNode();
    n.setValue(value);
}

void DBNodeH::propagateUpdate()
{
    DatabaseNode& n = getNode();
    n.propagateUpdate();
}

void DBNodeH::connectValueChanged(const void * receiver, const char* method)
{
    GVT_DEBUG(DBG_ALWAYS,"gvt::core::DBNodeH::connectValueChanged not implemented");
    //receiver->connect(&getNode(),SIGNAL(valueChanged()), method);
}

void DBNodeH::connectChildChanged(const void * receiver,  const char* method)
{
    GVT_DEBUG(DBG_ALWAYS,"gvt::core::DBNodeH::connectChildChanged not implemented");
    //receiver->connect(&getNode(),SIGNAL(connectChildChanged()), method);
}

Vector<DBNodeH> DBNodeH::getChildren() 
{
    CoreContext* ctx = CoreContext::instance();
    Database& db = *(ctx->database());
    Vector<DatabaseNode*> children = db.getChildren(UUID());
    Vector<DBNodeH> result;
    for(int i=0; i < children.size(); i++)
        result.push_back(DBNodeH(children[i]->UUID()));
    return result;
}
