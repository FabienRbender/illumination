#include "Header.h"
#include "dirent.h"
 
using namespace std;
using namespace cv;

int main(int argc, char **argv)
{
	string adresse_ref_img = "CCPPPatches.jpg";

	string adresse_rep = "examples";
	string adresse_rep_original = adresse_rep + "\\Original";
	string adresse_rep_colorchecker = adresse_rep + "\\ColorChecker";
	string adresse_rep_colorchecker_summary = adresse_rep + "\\ColorChecker_summary";
	string adresse_rep_colorchecker_tab = adresse_rep + "\\ColorChecker_tab";

	string adresse_rep_crope = adresse_rep + "\\Crope";
	string adresse_rep_crope_correction = adresse_rep + "\\Crope_color_correction";
	string adresse_rep_colorchecker_summary_correction = adresse_rep + "\\ColorChecker_summary_correction";
	string adresse_rep_color_correction_linear = adresse_rep + "\\pred_color_correction\\linear_model";

	Mat img_ref = imread(adresse_ref_img);
	if (img_ref.empty()) { exit(1); };
	Mat col_position = Mat::zeros(4,6,CV_32FC2);
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 6; j++) {
			col_position.at<Point2f>(i, j) = Point2f((i+1) * 40, (j+1) * 40);
		}
	}

	DIR* rep = NULL;
	dirent* fichier = NULL;
	vector<string> files_names;

	rep = opendir(adresse_rep_original.c_str());
	if (rep == NULL) { cout << "Repertoire non charge" << endl; exit(1); }

	cout << "Fichier a traiter:" << endl;
	while ((fichier = readdir(rep)) != NULL) {
		if (strcmp(fichier->d_name, ".") != 0 && strcmp(fichier->d_name, "..") != 0) {
			cout << fichier->d_name << endl;
			files_names.push_back(fichier->d_name);
		}
	}

	closedir(rep);

	bool find_colorchecker_bool = TRUE;
	bool apply_color_correction = FALSE;

	//cout << "Que voulez vous faire ?" << endl;
	//cout << "Find Colorchecker" << endl;
	//cin >> find_colorchecker_bool;
	//cout << "Apply color correction" << endl;
	//cin >> apply_color_correction;


	string* lines = new string[size(files_names) + 1];

	cout << endl;
	cout << "Début du Traitement :" << endl;
	
	

	if(find_colorchecker_bool){
		#pragma omp parallel for
		for (int i = 0; i < size(files_names); i++) {
			Mat img = imread(adresse_rep_original + "\\" + files_names[i]);
			if (img.empty()) { exit(1); };
			Mat colorchecker = find_colorchecker(img, img_ref);
			string fishname = files_names[i].substr(0, files_names[i].find("."));
			imwrite(adresse_rep_colorchecker + "\\" + fishname + ".jpg", colorchecker);
			Mat conteneur = Mat::zeros(col_position.size()*25, CV_8UC3);
			vector<Scalar> color_list = extract_color(colorchecker, col_position, conteneur);
			imwrite(adresse_rep_colorchecker_summary + "\\" + fishname + ".jpg", conteneur, {100});
			write_summary_csv(adresse_rep_colorchecker_tab + "\\" + fishname + ".txt", color_list);
			cout << adresse_rep + "\\" + files_names[i] << endl;
		}
	}


	if(apply_color_correction){
		////#pragma omp parallel for
		for (int i = 0; i < size(files_names); i++) {
			string fishname = files_names[i].substr(0, files_names[i].find("."));

			Mat img = imread(adresse_rep_crope + "\\" + fishname + ".jpg");
			Mat colorchecker = imread(adresse_rep_colorchecker_summary + "\\" + fishname + ".jpg");

			if (img.empty() | colorchecker.empty()) { cout << "ERROR PATH" << fishname << "IMAGE OR COLORCHECKER " << endl; system("PAUSE"); exit(1); };

			img.convertTo(img,CV_32FC3);
			colorchecker.convertTo(colorchecker, CV_32FC3);

			Mat img_split[3];
			Mat colorchecker_split[3];
			split(img, img_split);
			split(colorchecker, colorchecker_split);

			ifstream inputfile;
			inputfile.open(adresse_rep_color_correction_linear + "\\" + fishname + ".txt");
		
			string coeffB_tmp,coeffG_tmp,coeffR_tmp,interB_tmp,interG_tmp,interR_tmp,dummy;
			float coeffB,coeffG,coeffR,interB,interG,interR;

			getline(inputfile, dummy, '\n');
			getline(inputfile, interB_tmp, ';');

			interB = stof(interB_tmp.c_str());
			getline(inputfile, interG_tmp, ';');
			interG = stof(interG_tmp.c_str());
			getline(inputfile, interR_tmp, '\n');
			interR = stof(interR_tmp.c_str());

			getline(inputfile, coeffB_tmp, ';');
			coeffB = stof(coeffB_tmp.c_str());
			getline(inputfile, coeffG_tmp, ';');
			coeffG = stof(coeffG_tmp.c_str());
			getline(inputfile, coeffR_tmp, '\n');
			coeffR = stof(coeffR_tmp.c_str());

			img_split[0] = img_split[0] * coeffB + interB;
			img_split[1] = img_split[1] * coeffG + interG;
			img_split[2] = img_split[2] * coeffR + interR;
			colorchecker_split[0] = colorchecker_split[0] * coeffB + interB;;
			colorchecker_split[1] = colorchecker_split[1] * coeffG + interG;;
			colorchecker_split[2] = colorchecker_split[2] * coeffR + interR;
		
			Mat img_corrected;
			Mat colorchecker_corrected;
			merge(img_split,3, img_corrected);
			merge(colorchecker_split, 3, colorchecker_corrected);

			img_corrected.convertTo(img_corrected, CV_8UC3);
			colorchecker_corrected.convertTo(colorchecker_corrected, CV_8UC3);

			imwrite(adresse_rep_crope_correction + "\\" + fishname + ".jpg", img_corrected);
			imwrite(adresse_rep_colorchecker_summary_correction + "\\" + fishname + ".jpg", colorchecker_corrected);
			cout << fishname << endl;
		}
	}

	return 0;
}