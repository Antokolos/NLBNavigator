#include "nlb/domain/AbstractModifyingItem.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/domain/ModificationImpl.h"
#include "nlb/util/StringHelper.h"
#include "nlb/util/FileUtils.h"
#include "nlb/exception/NLBExceptions.h"

namespace {
    const std::string MODORDER_FILE_NAME = "modorder";
    const std::string MODORDER_SEPARATOR = "\n";
    const std::string DEFAULT_MODORDER = "";
}

AbstractModifyingItem::AbstractModifyingItem() 
    : AbstractIdentifiableItem() {
}

AbstractModifyingItem::AbstractModifyingItem(NonLinearBook* currentNLB)
    : AbstractIdentifiableItem(currentNLB) {
}

AbstractModifyingItem::AbstractModifyingItem(const ModifyingItem* modifyingItem,
                                           NonLinearBook* currentNLB)
    : AbstractIdentifiableItem(modifyingItem, currentNLB) {
    for (const auto& modification : modifyingItem->getModifications()) {
        if (!modification->isModificationImpl()) {
            continue;
        }
        auto modificationImpl = (ModificationImpl*) (modification);
        if (modificationImpl) {
            auto newMod = new ModificationImpl(modificationImpl, this, currentNLB);
            m_modifications.push_back(newMod);
        }
    }
}

std::vector<Modification*> AbstractModifyingItem::getModifications() const {
    std::vector<Modification*> result;
    result.reserve(m_modifications.size());
    for (const auto& mod : m_modifications) {
        result.push_back((Modification*) (mod));
    }
    return result;
}

bool AbstractModifyingItem::hasNoModifications() const {
    if (!m_modifications.empty()) {
        for (const auto& modification : m_modifications) {
            if (!modification->AbstractIdentifiableItem::isDeleted()) {
                return false;
            }
        }
    }
    return true;
}

Modification* AbstractModifyingItem::getModificationById(const std::string& modId) const {
    for (const auto& modification : m_modifications) {
        if (modId == modification->AbstractIdentifiableItem::getId()) {
            return (Modification*) (modification);
        }
    }
    return nullptr;
}

void AbstractModifyingItem::addModification(ModificationImpl* modification) {
    m_modifications.push_back(modification);
}

void AbstractModifyingItem::writeModifications(const FileManipulator* fileManipulator,
                                             const std::string& itemDir) {
    const std::string modificationsDir = FileUtils::combinePath(itemDir, "modifications");
    
    if (m_modifications.empty()) {
        if (FileUtils::exists(modificationsDir)) {
            fileManipulator->deleteFileOrDir(modificationsDir);
        }
    } else {
        fileManipulator->createDir(modificationsDir, 
            "Cannot create modifications directory for item with Id = " + this->getId());
            
        for (const auto& modification : m_modifications) {
            modification->writeModification(fileManipulator, modificationsDir);
        }
    }
}

void AbstractModifyingItem::readModifications(const std::string& itemDir) {
    std::string modOrderString = FileManipulator::getOptionalFileAsString(
        itemDir,
        MODORDER_FILE_NAME,
        DEFAULT_MODORDER
    );

    const std::string modsDir = FileUtils::combinePath(itemDir, "modifications");
    if (!FileUtils::exists(modsDir) && !modOrderString.empty()) {
        throw NLBIOException(
            "Invalid NLB structure: modifications directory does not exist for item with Id = " 
            + this->getId() 
            + ", but some modifications should be specified"
        );
    }

    m_modifications.clear();
    
    std::vector<std::string> modDirs = FileUtils::getDirectoryFiles(modsDir);
    if (modOrderString.empty()) {
        if (!modDirs.empty()) {
            throw NLBConsistencyException(
                "Inconsistent NLB structure: 'modifications' directory should be empty for item with id = "
                + this->getId()
            );
        }
    } else {
        std::vector<std::string> modOrderList = StringHelper::tokenize(modOrderString, MODORDER_SEPARATOR);
        std::vector<std::string> modDirsSortedList = createSortedDirList(modDirs, modOrderList);

        for (const auto& dir : modDirsSortedList) {
            auto modification = new ModificationImpl(this);
            modification->readModification(FileUtils::combinePath(modsDir, dir));
            m_modifications.push_back(modification);
        }
    }
}

void AbstractModifyingItem::writeModOrderFile(const FileManipulator* fileManipulator,
                                            const std::string& itemDir) {
    std::string content;
    const int lastElemIndex = static_cast<int>(m_modifications.size()) - 1;
    
    if (lastElemIndex >= 0) {
        for (int i = 0; i < lastElemIndex; i++) {
            const auto& modification = m_modifications[i];
            if (!modification->AbstractIdentifiableItem::isDeleted()) {
                content += modification->AbstractIdentifiableItem::getId() + MODORDER_SEPARATOR;
            }
        }
        
        if (!m_modifications[lastElemIndex]->AbstractIdentifiableItem::isDeleted()) {
            content += m_modifications[lastElemIndex]->AbstractIdentifiableItem::getId();
        }
        
        fileManipulator->writeOptionalString(itemDir, MODORDER_FILE_NAME, content, DEFAULT_MODORDER);
    } else {
        fileManipulator->writeOptionalString(itemDir, MODORDER_FILE_NAME, "", DEFAULT_MODORDER);
    }
}
