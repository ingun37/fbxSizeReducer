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
	if (argc != 5)
	{
		printf("invalid arg");
		return -1;
	}
	const char* filename = argv[1];
	const char* strStart = argv[2];
	const char* strTerm = argv[3];
	const char* strCnt = argv[4];
#else
	const char* filename = "PC@GUN.fbx";
	const char* strStart = "0.5";
	const char* strTerm = "0.1";
	const char* strCnt = "10";
#endif
	
	float rtStart, rtTerm;
	int rtCnt;
	puts("asdf");
	sscanf_s(strStart, "%f", &rtStart);
	sscanf_s(strTerm, "%f", &rtTerm);
	sscanf_s(strCnt, "%d", &rtCnt);
	puts("fdsa");
	printf("%f, %f, %d", rtStart, rtTerm, rtCnt);
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

	

	FbxScene* scene = FbxScene::Create(fm, "myscene");

	

	double factor = (double)rtStart;
	for (int fi = 0; fi < rtCnt; fi++)
	{
		FbxImporter* importer = FbxImporter::Create(fm, "");
		if (!importer->Initialize(filename, -1, fm->GetIOSettings()))
		{
			printf("error returned : %s\n", importer->GetStatus().GetErrorString());
			exit(-1);
		}

		importer->Import(scene);
		importer->Destroy();

		for (int i = 0; i < scene->GetNodeCount(); i++)
		{
			FbxNode* node = scene->GetNode(i);
			if (node->GetNodeAttribute())
			{
				if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::EType::eSkeleton)
				{
					output << "skelnode " << node->GetName() << " : ";
					FbxSkeleton* skeleton = node->GetSkeleton();
					FbxDouble3 trans = node->LclTranslation;
					output << endl << "    " << "original trans : " << trans[0] << ", " << trans[1] << ", " << trans[2];
					trans.mData[0] *= factor;
					trans.mData[1] *= factor;
					trans.mData[2] *= factor;
					output << endl << "    " << "scaleddd trans : " << trans[0] << ", " << trans[1] << ", " << trans[2] << endl;
					node->LclTranslation.Set(trans);
				}
				else
					output << endl;
			}
		}

		for (int i = 0; i < scene->GetNodeCount(); i++)
		{
			FbxNode* node = scene->GetNode(i);
			if (node->GetNodeAttribute())
			{
				if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::EType::eMesh)
				{
					output << "mesh node : " << node->GetName();
					FbxMesh* mesh = node->GetMesh();

					if (mesh == NULL)
					{
						output << "error mesh is null" << endl;
						continue;
					}
					output << ", mem usage : " << mesh->MemoryUsage() << ", deformer cnt : " << mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) << endl;
					int vcnt = mesh->GetControlPointsCount();
					FbxVector4* vs = mesh->GetControlPoints();
					
					for (int vi = 0; vi < vcnt; vi++)
					{
						vs[vi][0] *= factor;
						vs[vi][1] *= factor;
						vs[vi][2] *= factor;
					}

					FbxSkin* skin = (FbxSkin*)(mesh->GetDeformer(0, FbxDeformer::EDeformerType::eSkin));
					if (skin)
					{

						for (int cli = 0; cli < skin->GetClusterCount(); cli++)
						{
							FbxCluster* cluster = skin->GetCluster(cli);
							output << "\tcluster no." << cli << " has " << cluster->GetControlPointIndicesCount() << " connected verts" << endl;
							FbxNode* linkedskel = cluster->GetLink();
							if (linkedskel == NULL)
							{
								output << "cluster have no linked skel" << endl;
								continue;
							}
							if (linkedskel->GetNodeAttribute()->GetAttributeType() != FbxNodeAttribute::EType::eSkeleton)
							{
								output << "linked node is not skeleton type" << endl;
								continue;
							}
							cluster->SetTransformLinkMatrix(linkedskel->EvaluateGlobalTransform());
							cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
						}
					}
					else
						output << "mesh have no skin" << endl;

				}
				else
					output << endl;
			}
		}

		FbxExporter* exporter = FbxExporter::Create(fm, "");
		char outFBXName[128];
		sprintf_s(outFBXName, "out_%d", fi);
		bool exportstatus = exporter->Initialize(outFBXName, -1, fm->GetIOSettings());
		if (exportstatus == false)
		{
			puts("err export fail");
		}
		exporter->Export(scene);
		exporter->Destroy();
		factor += (double)rtTerm;
	}

	
	scene->Destroy();
	
	ios->Destroy();
	fm->Destroy();
	output.close();
	output2.close();
	output3.close();
	return 0;
}