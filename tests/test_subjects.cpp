// Written from tests\specs\subjects.md.

#include <filesystem>
#include <fstream>

#include "check.h"
#include "../src/subjects.h"

namespace {

namespace fs = std::filesystem;

fs::path checks_folder() {
	return fs::temp_directory_path() / L"crammy subjects checks";
}

void make_downloads( const fs::path &downloads ) {
	fs::create_directories( downloads / L"Math 117" );
	fs::create_directories( downloads / L"ECE 150" );
	std::ofstream( downloads / L"GENE 119.txt" ) << "not a folder";
}

vector<wstring> sample_titles() {
	return { L"ECE 150: Lecture", L"ECE 150: Exam", L"Math 117: Lecture",
	         L"Math 117: Tutorial", L"ECE 198" };
}

void check_desktop_names() {
	check( desktop_name_for( L"ECE 150" ) == L"150", "The desktop name for ECE 150 is 150." );
	check( desktop_name_for( L"MATH 117" ) == L"117", "The desktop name for MATH 117 is 117." );
	check( desktop_name_for( L"ENGL 192" ) == L"192", "The desktop name for ENGL 192 is 192." );
	check( desktop_name_for( L"GENE 119" ) == L"119", "The desktop name for GENE 119 is 119." );
	check( desktop_name_for( L"Math 115" ) == L"115", "The desktop name for Math 115 is 115." );
	check( desktop_name_for( L"Seminar" ) == L"", "A course with no digits has an empty desktop name." );
}

void check_same_course() {
	check( same_course( L"MATH 117", L"Math 117" ), "Course names that differ only in case are the same course." );
	check( same_course( L"ECE150", L"ECE 150" ), "Course names that differ only in spaces are the same course." );
	check( !same_course( L"ECE 150", L"ECE 105" ), "ECE 150 and ECE 105 are different courses." );
	check( !same_course( L"ECE 150", L"ECE 1500" ), "ECE 150 and ECE 1500 are different courses." );
}

void check_course_folders() {
	fs::path downloads = checks_folder() / L"downloads";
	fs::remove_all( checks_folder() );
	make_downloads( downloads );
	wstring folder = downloads.wstring();

	check( has_course_folder( L"MATH 117", folder ), "MATH 117 matches the folder named Math 117." );
	check( has_course_folder( L"ECE 150", folder ), "ECE 150 matches the folder named ECE 150." );
	check( !has_course_folder( L"GENE 119", folder ), "GENE 119 has no folder when only a file has that name." );
	check( !has_course_folder( L"ECE 190", folder ), "ECE 190 has no folder in the test downloads folder." );

	fs::remove_all( checks_folder() );
}

void check_setups_for_classes() {
	vector<wstring> titles = sample_titles();

	check( setup_for_class( L"ECE 150", L"LEC", titles ) == L"ECE 150: Lecture", "ECE 150 with LEC gives ECE 150: Lecture." );
	check( setup_for_class( L"MATH 117", L"TUT", titles ) == L"Math 117: Tutorial", "MATH 117 with TUT gives Math 117: Tutorial." );
	check( setup_for_class( L"MATH 117", L"LEC", titles ) == L"Math 117: Lecture", "MATH 117 with LEC gives Math 117: Lecture." );
	check( setup_for_class( L"ECE 150", L"TUT", titles ) == L"", "ECE 150 with TUT gives nothing because it has no tutorial setup." );
	check( setup_for_class( L"ECE 150", L"LAB", titles ) == L"", "ECE 150 with LAB gives nothing." );
	check( setup_for_class( L"ECE 150", L"TST", titles ) == L"", "ECE 150 with TST gives nothing." );
	check( setup_for_class( L"ECE 150", L"SEM", titles ) == L"", "ECE 150 with SEM gives nothing." );
	check( setup_for_class( L"ECE 198", L"LAB", titles ) == L"", "ECE 198 with LAB gives nothing." );
	check( setup_for_class( L"MATH 115", L"LEC", titles ) == L"", "MATH 115 with LEC gives nothing because it has no setups." );
	check( setup_for_class( L"ECE 150", L"LEC", {} ) == L"", "No setup titles give nothing." );

	vector<wstring> odd_titles = { L"math 117: LECTURE" };
	check( setup_for_class( L"Math 117", L"LEC", odd_titles ) == L"math 117: LECTURE", "The setup name is matched ignoring case and the title is returned as given." );
}

void check_kind_names() {
	check( kind_name( L"LEC" ) == L"lecture", "The name of kind LEC is lecture." );
	check( kind_name( L"TUT" ) == L"tutorial", "The name of kind TUT is tutorial." );
	check( kind_name( L"LAB" ) == L"lab", "The name of kind LAB is lab." );
	check( kind_name( L"TST" ) == L"test", "The name of kind TST is test." );
	check( kind_name( L"SEM" ) == L"seminar", "The name of kind SEM is seminar." );
	check( kind_name( L"PRJ" ) == L"prj", "An unknown kind comes back in lower case." );
}

} // namespace

void test_subjects() {
	check_desktop_names();
	check_same_course();
	check_course_folders();
	check_setups_for_classes();
	check_kind_names();
}
