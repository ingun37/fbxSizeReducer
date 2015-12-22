#define comp_level_horizon 0.01
#define comp_level_deriv 0.01
#define comp_level_integError 0.01
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <fbxsdk.h>
#include <math.h>
#include <iomanip>
using namespace std;

ofstream output, output2, output3;

int main(int argc, char **argv)
{
#ifndef _DEBUG
	if (argc != 2)
	{
		printf("invalid arg");
		return 0;
	}
	const char* filename = argv[1];
#else
	const char* filename = "PC@GUN.fbx";
#endif
	
	output.open("output.txt", ios::out | ios::trunc);
	output2.open("output2.txt", ios::out | ios::trunc);
	output3.open("output3.txt", ios::out | ios::trunc);
	if (!output.is_open())
	{
		exit(1);
	}
	FbxManager* fm = FbxManager::Create();
	FbxIOSettings *ios = FbxIOSettings::Create(fm, IOSROOT);
	//ios->SetBoolProp(EXP_FBX_ANIMATION, false);
	ios->SetIntProp(EXP_FBX_COMPRESS_LEVEL, 9);
	
	ios->SetAllObjectFlags(true);
	fm->SetIOSettings(ios);

	FbxImporter* importer = FbxImporter::Create(fm, "");
	if (!importer->Initialize(filename, -1, fm->GetIOSettings()))
	{
		printf("error returned : %s\n", importer->GetStatus().GetErrorString());
		exit(-1);
	}

	FbxScene* scene = FbxScene::Create(fm, "myscene");

	importer->Import(scene);
	importer->Destroy();
	output << "some\n";
	output << "charcnt : " << scene->GetCharacterCount() << endl << "node cnt : " << scene->GetNodeCount() << endl;
	int animstackcnt = scene->GetSrcObjectCount<FbxAnimStack>();
	output << "animstackcnt : " << animstackcnt << endl;

	output << "------------" << endl;
	vector<FbxNode*> removableNodes;

	for (int i = 0; i < scene->GetNodeCount(); i++)
	{
		FbxNode* node = scene->GetNode(i);
		output << "scene's node " << i << " : " << node->GetName() << ", childcnt : " << node->GetChildCount();
		if (node->GetNodeAttribute())
		{
			output <<", att type : " << node->GetNodeAttribute()->GetAttributeType();
			if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::EType::eMesh)
			{
				FbxMesh* mesh = node->GetMesh();

				output << ", mem usage : " << mesh->MemoryUsage() << ", deformer cnt : " << mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) << endl;

				for (int di = 0; di < mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin); di++)
				{
					FbxSkin* skinner = (FbxSkin*)mesh->GetDeformer(di, FbxDeformer::EDeformerType::eSkin);
					int ccnt = skinner->GetClusterCount();

					output << "skinner cluster count : " << ccnt << endl;
					mesh->RemoveDeformer(di);
				}
			}
			else
				output << endl;
		}
		else
		{
			output << ", att type : none" << endl;
		}
	}

	FbxExporter* exporter = FbxExporter::Create(fm, "");
	const char* outFBXName = "after.fbx";

	bool exportstatus = exporter->Initialize(outFBXName, -1, fm->GetIOSettings());
	if (exportstatus == false)
	{
		puts("err export fail");
	}
	exporter->Export(scene);
	exporter->Destroy();
	scene->Destroy();
	
	ios->Destroy();
	fm->Destroy();
	output.close();
	output2.close();
	output3.close();
	return 0;
}