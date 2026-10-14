#include "nlb/domain/ModificationImpl.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/api/NonLinearBook.h"
#include "nlb/api/Variable.h"
#include "nlb/api/Page.h"
#include "nlb/api/ModifyingItem.h"
#include "nlb/domain/SearchResult.h"
#include "nlb/exception/NLBExceptions.h"

const std::string ModificationImpl::EXTERNAL_FILE_NAME = "external";
const std::string ModificationImpl::VARID_FILE_NAME = "varid";
const std::string ModificationImpl::TYPE_FILE_NAME = "type";
const std::string ModificationImpl::EXPRID_FILE_NAME = "exprid";

const std::set<Modification::Type> ModificationImpl::UNPARAMETRIZED_TYPES = {
    Type::CNTNR,
    Type::CLONE,
    Type::PDSCS,
    Type::SSND,
    Type::WSND,
    Type::GETTAG,
    Type::CLEAR,
    Type::CLRINV,
    Type::ELSE,
    Type::END,
    Type::RETURN,
    Type::SNAPSHOT,
    Type::COUNTGET,
    Type::COUNTRST
};

ModificationImpl::ModificationImpl()
    : AbstractIdentifiableItem()
    , m_external(DEFAULT_EXTERNAL)
    , m_type(Type::ASSIGN)
    , m_varId("")
    , m_exprId("") {
}

ModificationImpl::ModificationImpl(const ModifyingItem* parent)
    : AbstractIdentifiableItem(parent->getCurrentNLB())
    , m_external(DEFAULT_EXTERNAL)
    , m_type(Type::ASSIGN)
    , m_varId("")
    , m_exprId("") {
    setParent(parent);
}

ModificationImpl::ModificationImpl(const Modification* modification,
                                 NonLinearBook* currentNLB)
    : AbstractIdentifiableItem(modification, currentNLB)
    , m_external(modification->isExternal())
    , m_type(modification->getType())
    , m_varId(modification->getVarId())
    , m_exprId(modification->getExprId()) {
}

ModificationImpl::ModificationImpl(const Modification* modification,
                                 const ModifyingItem* parent,
                                 NonLinearBook* currentNLB)
    : AbstractIdentifiableItem(modification, parent, currentNLB)
    , m_external(modification->isExternal())
    , m_type(modification->getType())
    , m_varId(modification->getVarId())
    , m_exprId(modification->getExprId()) {
}

void ModificationImpl::copy(const Modification* modification) {
    AbstractIdentifiableItem::copy(modification);
    m_external = modification->isExternal();
    m_type = modification->getType();
    m_varId = modification->getVarId();
    m_exprId = modification->getExprId();
}

bool ModificationImpl::isExternal() const {
    return m_external;
}

void ModificationImpl::setExternal(bool external) {
    m_external = external;
}

std::string ModificationImpl::getVarId() const {
    return m_varId;
}

void ModificationImpl::setVarId(const std::string& varId) {
    m_varId = varId;
}

std::string ModificationImpl::getExprId() const {
    return m_exprId;
}

void ModificationImpl::setExprId(const std::string& exprId) {
    m_exprId = exprId;
}

Modification::Type ModificationImpl::getType() const {
    return m_type;
}

bool ModificationImpl::returnsValue() const {
    switch (m_type) {
        case Type::CNTNR:
        case Type::CLONE:
        case Type::GETTAG:
        case Type::ID:
        case Type::ASSIGN:
        case Type::HAVE:
        case Type::POP:
        case Type::EJECT:
        case Type::SIZE:
        case Type::RND:
        case Type::DSC:
        case Type::ACTT:
        case Type::OBJS:
        case Type::ACHIEVED:
        case Type::COUNTGET:
            return true;
        default:
            return false;
    }
}

bool ModificationImpl::isParametrized() const {
    return UNPARAMETRIZED_TYPES.find(m_type) == UNPARAMETRIZED_TYPES.end();
}

SearchResult* ModificationImpl::searchText(const SearchContract& contract) const {
    auto result = AbstractIdentifiableItem::searchText(contract);
    if (result) {
        return result;
    }

    if (!AbstractIdentifiableItem::isDeleted()) {
        auto parent = AbstractIdentifiableItem::getParent();
        auto variable = AbstractIdentifiableItem::getCurrentNLB()->getVariableById(m_varId);
        
        if (variable) {
            auto resultV = variable->searchText(contract);
            if (resultV && parent) {
                resultV->setId(parent->getId());
                resultV->setModulePageId(parent->getCurrentNLB()->getParentPage()->getId());
                return resultV;
            }
        }

        auto expression = AbstractIdentifiableItem::getCurrentNLB()->getVariableById(m_exprId);
        if (expression) {
            auto resultE = expression->searchText(contract);
            if (resultE && parent) {
                resultE->setId(parent->getId());
                resultE->setModulePageId(parent->getCurrentNLB()->getParentPage()->getId());
                return resultE;
            }
        }
    }
    
    return nullptr;
}

void ModificationImpl::setType(const std::string& type) {
    // Using if-else chain to match Java implementation style
    if (type == "WHILE") m_type = Type::WHILE;
    else if (type == "TAG") m_type = Type::TAG;
    else if (type == "GETTAG") m_type = Type::GETTAG;
    else if (type == "IF") m_type = Type::IF;
    else if (type == "IFHAVE") m_type = Type::IFHAVE;
    else if (type == "ELSE") m_type = Type::ELSE;
    else if (type == "ELSEIF") m_type = Type::ELSEIF;
    else if (type == "END") m_type = Type::END;
    else if (type == "RETURN") m_type = Type::RETURN;
    else if (type == "HAVE") m_type = Type::HAVE;
    else if (type == "CLONE") m_type = Type::CLONE;
    else if (type == "CNTNR") m_type = Type::CNTNR;
    else if (type == "ID") m_type = Type::ID;
    else if (type == "ADD") m_type = Type::ADD;
    else if (type == "ADDU") m_type = Type::ADDU;
    else if (type == "ADDINV") m_type = Type::ADDINV;
    else if (type == "ADDALL") m_type = Type::ADDALL;
    else if (type == "ADDALLU") m_type = Type::ADDALLU;
    else if (type == "REMOVE") m_type = Type::REMOVE;
    else if (type == "RMINV") m_type = Type::RMINV;
    else if (type == "CLEAR") m_type = Type::CLEAR;
    else if (type == "CLRINV") m_type = Type::CLRINV;
    else if (type == "OBJS") m_type = Type::OBJS;
    else if (type == "SSND") m_type = Type::SSND;
    else if (type == "WSND") m_type = Type::WSND;
    else if (type == "SND") m_type = Type::SND;
    else if (type == "SPUSH") m_type = Type::SPUSH;
    else if (type == "WPUSH") m_type = Type::WPUSH;
    else if (type == "PUSH") m_type = Type::PUSH;
    else if (type == "POP") m_type = Type::POP;
    else if (type == "SINJECT") m_type = Type::SINJECT;
    else if (type == "INJECT") m_type = Type::INJECT;
    else if (type == "EJECT") m_type = Type::EJECT;
    else if (type == "SHUFFLE") m_type = Type::SHUFFLE;
    else if (type == "PRN") m_type = Type::PRN;
    else if (type == "DSC") m_type = Type::DSC;
    else if (type == "PDSC") m_type = Type::PDSC;
    else if (type == "PDSCS") m_type = Type::PDSCS;
    else if (type == "ACT") m_type = Type::ACT;
    else if (type == "ACTT") m_type = Type::ACTT;
    else if (type == "ACTF") m_type = Type::ACTF;
    else if (type == "USE") m_type = Type::USE;
    else if (type == "SIZE") m_type = Type::SIZE;
    else if (type == "RND") m_type = Type::RND;
    else if (type == "ACHMAX") m_type = Type::ACHMAX;
    else if (type == "ACHIEVE") m_type = Type::ACHIEVE;
    else if (type == "ACHIEVED") m_type = Type::ACHIEVED;
    else if (type == "GOTO") m_type = Type::GOTO;
    else if (type == "SNAPSHOT") m_type = Type::SNAPSHOT;
    else if (type == "COUNTGET") m_type = Type::COUNTGET;
    else if (type == "COUNTRST") m_type = Type::COUNTRST;
    else if (type == "OPENURL") m_type = Type::OPENURL;
    else if (type == "WINGEOM") m_type = Type::WINGEOM;
    else if (type == "INVGEOM") m_type = Type::INVGEOM;
    else if (type == "WINCOLOR") m_type = Type::WINCOLOR;
    else if (type == "INVCOLOR") m_type = Type::INVCOLOR;
    else m_type = Type::ASSIGN;
}

void ModificationImpl::setType(const Type& type) {
    m_type = type;
}

void ModificationImpl::writeModification(const FileManipulator* fileManipulator,
                                       const std::string& modificationsDir) {
    const std::string modificationDir = modificationsDir + "/" + AbstractIdentifiableItem::getId();
    
    if (AbstractIdentifiableItem::isDeleted()) {
        fileManipulator->deleteFileOrDir(modificationDir);
    } else {
        fileManipulator->createDir(
            modificationDir,
            "Cannot create NLB modification directory for modification with Id = " + AbstractIdentifiableItem::getId()
        );
        
        fileManipulator->writeOptionalString(
            modificationDir,
            EXTERNAL_FILE_NAME,
            std::to_string(m_external),
            std::to_string(DEFAULT_EXTERNAL)
        );
        
        fileManipulator->writeRequiredString(modificationDir, VARID_FILE_NAME, m_varId);
        fileManipulator->writeRequiredString(modificationDir, TYPE_FILE_NAME, Modification::typeToString(m_type)); // ✅ ИСПРАВЛЕНО!
        fileManipulator->writeRequiredString(modificationDir, EXPRID_FILE_NAME, m_exprId);
    }
}

void ModificationImpl::readModification(const std::string& modificationDir) {
    setId(modificationDir.substr(modificationDir.find_last_of("/\\") + 1));
    
    m_external = FileManipulator::getOptionalFileAsString(
        modificationDir,
        EXTERNAL_FILE_NAME,
        std::to_string(DEFAULT_EXTERNAL)
    ) == "true";

    m_varId = FileManipulator::getRequiredFileAsString(
        modificationDir,
        VARID_FILE_NAME,
        "Error while reading modification variable Id for modification with Id = " + AbstractIdentifiableItem::getId()
    );

    std::string type = FileManipulator::getRequiredFileAsString(
        modificationDir,
        TYPE_FILE_NAME,
        "Error while reading modification type for modification with Id = " + AbstractIdentifiableItem::getId()
    );
    
    setType(type);

    m_exprId = FileManipulator::getRequiredFileAsString(
        modificationDir,
        EXPRID_FILE_NAME,
        "Error while reading modification expression Id for modification with Id = " + AbstractIdentifiableItem::getId()
    );
}

std::string ModificationImpl::toString() const {
    return m_varId + (m_exprId.empty() ? "" : " = " + m_exprId);
}

std::string ModificationImpl::toStringWithType() const {
    std::string typeStr = Modification::typeToString(m_type);
    return typeStr + " " + toString();
}

bool ModificationImpl::equals(const Modification* other) const {
    if (!other) return false;
    if (this == other) return true;
    
    return m_external == other->isExternal() &&
           m_type == other->getType() &&
           m_varId == other->getVarId() &&
           m_exprId == other->getExprId();
}

bool ModificationImpl::operator==(const ModificationImpl& other) const {
    if (this == &other) return true;
    
    return m_external == other.m_external &&
           m_type == other.m_type &&
           m_varId == other.m_varId &&
           m_exprId == other.m_exprId;
}

size_t ModificationImpl::hash() const {
    size_t result = std::hash<bool>{}(m_external);
    result = 31 * result + std::hash<int>{}(static_cast<int>(m_type));
    result = 31 * result + std::hash<std::string>{}(m_varId);
    result = 31 * result + std::hash<std::string>{}(m_exprId);
    return result;
}
