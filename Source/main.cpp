#include "shunting-yard.h"
#include "builtin-features.inc"

#include "nlb/domain/NLBReader.h"
#include "nlb/domain/NLBExplorer.h"
#include "nlb/api/ConsoleProgressData.h"

/**
 * @brief Main function demonstrating NLB Reader usage
 */
int main(int argc, char* argv[]) {

    cparse_startup();

    GlobalScope::default_global()["x"] = 10;
    std::cout << calculator::calculate("'Hello ' + 'World'") << std::endl;
    std::cout << calculator::calculate("x + 1", GlobalScope::default_global()) << std::endl;

    system("chcp 65001");
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_nlb_directory> [mode]" << std::endl;
        std::cout << "Modes: info (default) | explore" << std::endl;
        return 1;
    }
    
    std::string nlbPath = argv[1];
    std::string mode = (argc > 2) ? argv[2] : "info";
    
    if (mode == "explore") {
        auto book = new NonLinearBookImpl();
        ConsoleProgressData progressData;
        book->load(nlbPath, progressData);
        std::cout << "Loaded: " << book->getTitle() << " by " << book->getAuthor() << std::endl;
        NLBExplorer explorer(book, book->getStartPoint());
        explorer.explore();
    } else {
        NLBReader reader;
        reader.readBook(nlbPath);
    }
    
    return 0;
}
