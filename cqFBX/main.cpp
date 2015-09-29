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

void tossScaleToChild(FbxNode* node, float x, float y, float z)
{
	FbxDouble3 trans = node->LclTranslation;

	trans.mData[0] *= x;
	trans.mData[1] *= y;
	trans.mData[2] *= z;

	node->LclTranslation.Set(trans);
	
	FbxDouble3 scale = node->LclScaling;

	x *= scale.mData[0];
	y *= scale.mData[1];
	z *= scale.mData[2];

	scale.mData[0] = scale.mData[1] = scale.mData[2] = 1;

	node->LclScaling.Set(scale);

	int ccnt = node->GetChildCount();
	for( int i = 0 ; i < ccnt ; i++ )
	{
		tossScaleToChild(node->GetChild(i), x, y, z);
	}

	if(node->GetNodeAttribute())
	{

		FbxNodeAttribute::EType att =  node->GetNodeAttribute()->GetAttributeType();
		if(att == FbxNodeAttribute::EType::eMesh)
		{
			FbxMesh* mesh = node->GetMesh();

			if (mesh == NULL)
			{
				output << "error mesh is null in tossscaletochild" << endl;
				return;
			}

			int vcnt = mesh->GetControlPointsCount();
			FbxVector4* vs = mesh->GetControlPoints();

			for (int vi = 0; vi < vcnt; vi++)
			{
				vs[vi][0] *= x;
				vs[vi][1] *= y;
				vs[vi][2] *= z;
			}

		}

	}
}

int main(int argc, char **argv)
{
#ifndef _DEBUG
	if (argc != 2)
	{
		printf("invalid arg");
		return -1;
	}
	const char* filename = argv[1];
#else
	const char* filename = "C_Wear_A_006.fbx";
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

	FbxScene* scene = FbxScene::Create(fm, "myscene");

	FbxImporter* importer = FbxImporter::Create(fm, "");
	if (!importer->Initialize(filename, -1, fm->GetIOSettings()))
	{
		printf("error returned : %s\n", importer->GetStatus().GetErrorString());
		exit(-1);
	}

	importer->Import(scene);
	importer->Destroy();

	FbxNode* rootnode = scene->GetRootNode();

	tossScaleToChild(rootnode, 1, 1, 1);


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
					output << "error mesh is null in  main" << endl;
					continue;
				}
				output << ", mem usage : " << mesh->MemoryUsage() << ", deformer cnt : " << mesh->GetDeformerCount(FbxDeformer::EDeformerType::eSkin) << endl;

				FbxSkin* skin = (FbxSkin*)(mesh->GetDeformer(0, FbxDeformer::EDeformerType::eSkin));
				if (!skin)
				{
					output << "mesh have no skin";
					continue;
				}
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
				output << endl;
		}
	}

	FbxExporter* exporter = FbxExporter::Create(fm, "");
	char* outFBXName = "normalized";
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