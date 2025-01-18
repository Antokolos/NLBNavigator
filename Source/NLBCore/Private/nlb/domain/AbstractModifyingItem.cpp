#include "nlb/domain/AbstractModifyingItem.h"
#include "nlb/util/FileManipulator.h"
#include "nlb/domain/ModificationImpl.h"
#include "nlb/util/StringHelper.h"
#include "nlb/exception/NLBExceptions.h"

namespace {
    const std::string MODORDER_FILE_NAME = "modorder";
    const std::string MODORDER_SEPARATOR = "\n";
    const std::string DEFAULT_MODORDER = "";
}

AbstractModifyingItem::AbstractModifyingItem() 
    : AbstractIdentifiableItem() {
}

AbstractModifyingItem::AbstractModifyingItem(std::shared_ptr<NonLinearBook> currentNLB)
    : AbstractIdentifiableItem(currentNLB) {
}

AbstractModifyingItem::AbstractModifyingItem(const std::shared_ptr<ModifyingItem>& modifyingItem, 
                                           std::shared_ptr<NonLinearBook> currentNLB)
    : AbstractIdentifiableItem(modifyingItem, currentNLB) {
    for (const auto& modification : modifyingItem->getModifications()) {
        auto modificationImpl = std::make_shared<ModificationImpl>(modification, shared_from_this(), currentNLB);
        m_modifications.push_back(modificationImpl);
    }
}

std::vector<std::shared_ptr<Modification>> AbstractModifyingItem::getModifications() const {
    std::vector<std::shared_ptr<Modification>> result;
    result.reserve(m_modifications.size());
    for (const auto& mod : m_modifications) {
        result.push_back(mod);
    }
    return result;
}

bool AbstractModifyingItem::hasNoModifications() const {
    if (!m_modifications.empty()) {
        for (const auto& modification : m_modifications) {
            if (!modification->isDeleted()) {
                return false;
            }
        }
    }
    return true;
}

std::shared_ptr<Modification> AbstractModifyingItem::getModificationById(const std::string& modId) const {
    for (const auto& modification : m_modifications) {
        if (modId == modification->getId()) {
            return modification;
        }
    }
    return nullptr;
}

void AbstractModifyingItem::addModification(std::shared_ptr<ModificationImpl> modification) {
    m_modifications.push_back(modification);
}

void AbstractModifyingItem::writeModifications(const std::shared_ptr<FileManipulator>& fileManipulator,
                                             const std::string& itemDir) {
    const std::string modificationsDir = itemDir + "/modifications";
    
    if (m_modifications.empty()) {
        if (fileManipulator->exists(modificationsDir)) {
            fileManipulator->deleteFileOrDir(modificationsDir);
        }
    } else {
        fileManipulator->createDir(modificationsDir, 
            "Cannot create modifications directory for item with Id = " + getId());
            
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

    const std::string modsDir = itemDir + "/modifications";
    if (!FileManipulator::exists(modsDir) && !modOrderString.empty()) {
        throw NLBIOException(
            "Invalid NLB structure: modifications directory does not exist for item with Id = " 
            + getId() 
            + ", but some modifications should be specified"
        );
    }

    m_modifications.clear();
    
    std::vector<std::string> modDirs;
    if (!FileManipulator::listDirectory(modsDir, modDirs)) {
        if (modOrderString.empty()) {
            return;
        } else {
            throw NLBIOException(
                "Error when enumerating modifications' directory contents for item with Id = " + getId()
            );
        }
    }

    if (modOrderString.empty()) {
        if (!modDirs.empty()) {
            throw NLBConsistencyException(
                "Inconsistent NLB structure: 'modifications' directory should be empty for item with id = "
                + getId()
            );
        }
    } else {
        std::vector<std::string> modOrderList = StringHelper::split(modOrderString, MODORDER_SEPARATOR);
        std::vector<std::string> modDirsSortedList = createSortedDirList(modDirs, modOrderList);

        for (const auto& dir : modDirsSortedList) {
            auto modification = std::make_shared<ModificationImpl>(shared_from_this());
            modification->readModification(dir);
            m_modifications.push_back(modification);
        }
    }
}

void AbstractModifyingItem::writeModOrderFile(const std::shared_ptr<FileManipulator>& fileManipulator,
                                            const std::string& itemDir) {
    std::string content;
    const int lastElemIndex = static_cast<int>(m_modifications.size()) - 1;
    
    if (lastElemIndex >= 0) {
        for (int i = 0; i < lastElemIndex; i++) {
            const auto& modification = m_modifications[i];
            if (!modification->isDeleted()) {
                content += modification->getId() + MODORDER_SEPARATOR;
            }
        }
        
        if (!m_modifications[lastElemIndex]->isDeleted()) {
            content += m_modifications[lastElemIndex]->getId();
        }
        
        fileManipulator->writeOptionalString(itemDir, MODORDER_FILE_NAME, content, DEFAULT_MODORDER);
    } else {
        fileManipulator->writeOptionalString(itemDir, MODORDER_FILE_NAME, "", DEFAULT_MODORDER);
    }
}