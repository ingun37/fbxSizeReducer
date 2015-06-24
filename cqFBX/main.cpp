#include <vector>
#include <iostream>
#include <fstream>
#include <fbxsdk.h>
#include <math.h>
using namespace std;

int main(int argc, char **argv)
{
	const char* filename = "PC@GUN.fbx";

	ofstream output, output2, output3;
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
				FbxSkin* skin = (FbxSkin*) (mesh->GetDeformer(0, FbxDeformer::EDeformerType::eSkin));
				for (int cli = 0; cli < skin->GetClusterCount(); cli++)
				{
					FbxCluster* cluster = skin->GetCluster(cli);
					output << "\tcluster no." << cli << " has " << cluster->GetControlPointIndicesCount() << " connected verts" << endl;
					//cluster->
					//skin->RemoveCluster(cluster);효과없음
				}
				if (mesh->IsTriangleMesh())
				{
					output << "\tit's triangle mesh" << endl;
					
				}
				//mesh->RemoveDeformer(0);효과없음
			}
			else
				output << endl;
		}
		else
		{
			output << ", att type : none" << endl;
		}
	}
	output << "-----------animinfo" << endl;
	int cubic = 0, linear = 0, cons = 0;
	for (int si = 0; si < animstackcnt; si++)
	{
		FbxAnimStack* stack = scene->GetSrcObject<FbxAnimStack>(si);
		for (int i = 0; i < stack->GetMemberCount<FbxAnimLayer>(); i++)
		{
			FbxAnimLayer* layer = stack->GetMember<FbxAnimLayer>(i);
			int curvenodecnt = layer->GetMemberCount<FbxAnimCurveNode>();
			int compositcnt = 0;
			for (int j = 0; j < curvenodecnt; j++)
			{
				FbxAnimCurveNode* cnode = layer->GetMember<FbxAnimCurveNode>(j);
				compositcnt += (cnode->IsComposite() ? 1 : 0);
			}
			output << "\tanimstack's layer " << i << " : " << layer->GetName() << ", curve node cnt : " << curvenodecnt << ", composit node cnt : " << compositcnt << endl;
			vector<FbxAnimCurveNode*> nodes2del;
			
			for (int j = 0; j < curvenodecnt; j++)
			{
				FbxAnimCurveNode* cnode = layer->GetMember<FbxAnimCurveNode>(j);
				output << "\t\tcurvenode " << j << " channel cnt : " << cnode->GetChannelsCount() << ", dst obj cnt " << cnode->GetDstObjectCount() << "(";
				for (int dsti = 0; dsti < cnode->GetDstObjectCount(); dsti++)
				{
					output << "," << cnode->GetDstObject(dsti)->GetName();
					if (cnode->GetDstObject(dsti)->GetSrcObjectCount() > 0)
						output << "<" << cnode->GetDstObject(dsti)->GetSrcObjectCount<FbxSkeleton>() << ">";
				}
				output << ")";
				FbxTimeSpan interval;
				if (cnode->GetAnimationInterval(interval))
				{
					output << ", start : " << interval.GetStart().GetTimeString() << ", end : " << interval.GetStop().GetTimeString() << endl;
				}
				else
				{
					nodes2del.push_back(cnode);
					output << ", no interval" << endl;
				}

				for (int chi = 0; chi < cnode->GetChannelsCount(); chi++)
				{
					
					int curvecnt = cnode->GetCurveCount(chi);
					output << "\t\t\tchannel." << chi << " curvecnt : " << curvecnt << endl;
					for (int ci = 0; ci < curvecnt; ci++)
					{
						FbxAnimCurve* curve = cnode->GetCurve(chi, ci);
						int keycnt = curve->KeyGetCount();
						output << "\t\t\t\tcurve no." << ci << " : key count : " << keycnt;
						output2 << "curve  " << endl;
						
						vector<int> keys2Remove;
						for (int cki = 0; cki < keycnt; cki++)
						{
							FbxAnimCurveKey prevkey, currkey, nextkey;

							if (cki == 0 || cki == keycnt - 1)
								continue;
							
							currkey = curve->KeyGet(cki);

							{
								if (currkey.GetInterpolation() == FbxAnimCurveDef::EInterpolationType::eInterpolationConstant)
									cons++;
								if (currkey.GetInterpolation() == FbxAnimCurveDef::EInterpolationType::eInterpolationCubic)
									cubic++;
								if (currkey.GetInterpolation() == FbxAnimCurveDef::EInterpolationType::eInterpolationLinear)
									linear++;
							}
							
							prevkey = curve->KeyGet(cki-1);
							nextkey = curve->KeyGet(cki + 1);
							FbxTime prevt = prevkey.GetTime();
							FbxTime currt = currkey.GetTime();
							FbxTime nextt = nextkey.GetTime();

							bool needit = false;
							float prevEv;

							FbxLongLong tmpll = (currt - prevt).GetMilliSeconds()/3;
							FbxTime termt;
							
							for (int termi = 0; termi < 3; termi++)
							{
								termt.SetMilliSeconds(tmpll*termi);
								float tmpf = curve->Evaluate(prevt + termt);
								output2 << "time:" << (prevt + termt).GetTimeString() << "\t\tvalue : " << curve->Evaluate(prevt + termt) << "(premid)";
								if (termi > 0)
								{
									if (abs(prevEv - tmpf) > 0.0001)
									{
										output2 << "(true)";
										needit = true;
										//break;
									}
								}
								output2 << endl;
								prevEv = tmpf;
							}

							output2 << "time:" << currt.GetTimeString() << "\t\tvalue : " << curve->Evaluate(currt) << "(key)" << "interpolation kind : " << ((int)currkey.GetInterpolation()) << endl;

							tmpll = (nextt - currt).GetMilliSeconds() / 3;
							if (needit == false)
							{
								for (int termi = 0; termi < 3; termi++)
								{
									termt.SetMilliSeconds(tmpll*termi);
									float tmpf = curve->Evaluate(currt + termt);
									output2 << "time:" << (currt + termt).GetTimeString() << "\t\tvalue : " << curve->Evaluate(currt + termt) << "(postmid)";
									if (abs(prevEv - tmpf) > 0.0001)
									{
										output2 << "(true)";
										needit = true;
										//break;
									}
									output2 << endl;
									prevEv = tmpf;
								}
							}

							if (!needit)
							{
								if (!(currkey.GetInterpolation() == FbxAnimCurveDef::EInterpolationType::eInterpolationConstant && nextkey.GetInterpolation() != FbxAnimCurveDef::EInterpolationType::eInterpolationConstant))
								keys2Remove.push_back(cki);
							}
						}
						for (int kri = keys2Remove.size() - 1; kri >= 0; kri--)
						{
							
							curve->KeyRemove(keys2Remove[kri]);
						}
						output2 << endl;
						//output << ", cubic:linear:const : " << cubic << ":" << linear << ":" << cons << endl;
						if (keys2Remove.size() > 0)
							output << ", " << keys2Remove.size() << " keys removed";

						keycnt = curve->KeyGetCount();
						output3 << "new keycnt for curve no." << ci << " is " << keycnt << endl;
						for (int cki = 0; cki < keycnt; cki++)
						{
							FbxAnimCurveKey prevkey, currkey, nextkey;

							if (cki == 0 || cki == keycnt - 1)
								continue;

							currkey = curve->KeyGet(cki);

							prevkey = curve->KeyGet(cki - 1);
							nextkey = curve->KeyGet(cki + 1);
							FbxTime prevt = prevkey.GetTime();
							FbxTime currt = currkey.GetTime();
							FbxTime nextt = nextkey.GetTime();

							bool needit = false;
							float prevEv;

							FbxLongLong tmpll = (currt - prevt).GetMilliSeconds() / 3;
							FbxTime termt;

							for (int termi = 0; termi < 3; termi++)
							{
								termt.SetMilliSeconds(tmpll*termi);
								float tmpf = curve->Evaluate(prevt + termt);
								output3 << "time:" << (prevt + termt).GetTimeString() << "\t\tvalue : " << curve->Evaluate(prevt + termt) << "(premid)";
								if (termi > 0)
								{
									if (abs(prevEv - tmpf) > 0.0001)
									{
										output3 << "(true)";
										needit = true;
										//break;
									}
								}
								output3 << endl;
								prevEv = tmpf;
							}

							output3 << "time:" << currt.GetTimeString() << "\t\tvalue : " << curve->Evaluate(currt) << "(key)" << "interpolation kind : " << ((int)currkey.GetInterpolation()) << endl;

							tmpll = (nextt - currt).GetMilliSeconds() / 3;
							if (needit == false)
							{
								for (int termi = 0; termi < 3; termi++)
								{
									termt.SetMilliSeconds(tmpll*termi);
									float tmpf = curve->Evaluate(currt + termt);
									output3 << "time:" << (currt + termt).GetTimeString() << "\t\tvalue : " << curve->Evaluate(currt + termt) << "(postmid)";
									if (abs(prevEv - tmpf) > 0.0001)
									{
										output3 << "(true)";
										needit = true;
										//break;
									}
									output3 << endl;
									prevEv = tmpf;
								}
							}
						}
					}

				}
			}
			//이부분은 별로 효과없음
			/*
			for (int di = 0; di < nodes2del.size(); di++)
			{
				layer->RemoveMember(nodes2del[di]);
			}
			/**/
			
		}
	}
	output << "cubic:linear:const  " << cubic << ":" << linear << ":" << cons << endl;
	FbxExporter* exporter = FbxExporter::Create(fm, "");
	const char* outFBXName = "rd.fbx";

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