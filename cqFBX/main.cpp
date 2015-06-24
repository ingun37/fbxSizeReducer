#define comp_level_horizon 0.01
#define comp_level_deriv 0.01
#define comp_level_integError 0.01
#include <vector>
#include <iostream>
#include <fstream>
#include <fbxsdk.h>
#include <math.h>
#include <iomanip>
using namespace std;

ofstream output, output2, output3;

bool slopkeepTest(FbxAnimCurve* curve, FbxAnimCurveKey prevkey, FbxAnimCurveKey currkey, FbxAnimCurveKey nextkey)
{
	FbxTime prevt = prevkey.GetTime();
	FbxTime currt = currkey.GetTime();
	FbxTime nextt = nextkey.GetTime();

	float slope1 = (currkey.GetValue() - prevkey.GetValue()) / (currt.GetSecondDouble() - prevt.GetSecondDouble());
	float slope2 = (nextkey.GetValue() - currkey.GetValue()) / (nextt.GetSecondDouble() - currt.GetSecondDouble());
	output2 << setw(20) << "slope : " << setw(10) << setprecision(3) << slope1 << "," << setw(10) << slope2;
	if (abs(slope1 - slope2) > 0.005)
	{
		output2 << "  keepinSlope" << endl;
		return true;
	}
	output2 << endl;

	float deriv1, deriv2, deriv3, deriv4;
	deriv1 = prevkey.GetDataFloat(FbxAnimCurveDef::EDataIndex::eRightSlope);
	deriv2 = prevkey.GetDataFloat(FbxAnimCurveDef::EDataIndex::eNextLeftSlope);
	deriv3 = currkey.GetDataFloat(FbxAnimCurveDef::EDataIndex::eRightSlope);
	deriv4 = currkey.GetDataFloat(FbxAnimCurveDef::EDataIndex::eNextLeftSlope);
		
	FbxTime mili;
	mili.SetMilliSeconds(1);
	output3 << "deriv1 : " << deriv1 << ", val : " << prevkey.GetValue() << ", +1mili value : " << curve->EvaluateRightDerivative(prevt + mili) << endl;
	float derivl11, derivl12, derivl21, derivl22;
	float derivr11, derivr12, derivr21, derivr22;

	FbxTime term1, term2;
	term1.SetSecondDouble((currt.GetSecondDouble() - prevt.GetSecondDouble()) / 3);
	term2.SetSecondDouble((nextt.GetSecondDouble() - currt.GetSecondDouble()) / 3);

	derivl11 = curve->EvaluateLeftDerivative(prevt + term1);
	derivl12 = curve->EvaluateLeftDerivative(prevt + term1 + term1);
	derivl21 = curve->EvaluateLeftDerivative(currt + term2);
	derivl22 = curve->EvaluateLeftDerivative(currt + term2 + term2);

	derivr11 = curve->EvaluateRightDerivative(prevt + term1);
	derivr12 = curve->EvaluateRightDerivative(prevt + term1 + term1);
	derivr21 = curve->EvaluateRightDerivative(currt + term2);
	derivr22 = curve->EvaluateRightDerivative(currt + term2 + term2);

		
	output2 << setw(20) << "deriv : " << setprecision(3)
		<< deriv1  <<","
		<< deriv2  <<","
		<< deriv3  <<","
		<< deriv4  <<","
		<< derivl11<<","
		<< derivl12<<","
		<< derivl21<<","
		<< derivl22<<","
		<< derivr11<<","
		<< derivr12<<","
		<< derivr21<<","
		<< derivr22<<","
		;

	if (
		prevkey.GetInterpolation() == FbxAnimCurveDef::eInterpolationConstant &&
		currkey.GetInterpolation() == FbxAnimCurveDef::eInterpolationConstant &&
		nextkey.GetInterpolation() == FbxAnimCurveDef::eInterpolationConstant
		)
	{
		if (
			abs(prevkey.GetValue() - currkey.GetValue()) < comp_level_horizon &&
			abs(currkey.GetValue() - nextkey.GetValue()) < comp_level_horizon
			)
			return false;
	}
	else if (
		prevkey.GetInterpolation() == FbxAnimCurveDef::eInterpolationCubic &&
		currkey.GetInterpolation() == FbxAnimCurveDef::eInterpolationCubic &&
		nextkey.GetInterpolation() == FbxAnimCurveDef::eInterpolationCubic
		)
	{
		if (
			abs(deriv1 - deriv2) < comp_level_deriv &&
			abs(deriv2 - deriv3) < comp_level_deriv &&
			abs(deriv3 - deriv4) < comp_level_deriv
			)
		{
			float nearCurr = (deriv1*(currt.GetMilliSeconds() - prevt.GetMilliSeconds())) + prevkey.GetValue();
			float nearNext = (deriv3*(nextt.GetMilliSeconds() - currt.GetMilliSeconds())) + currkey.GetValue();
			if (
				abs(nearCurr - currkey.GetValue()) < comp_level_integError &&
				abs(nearNext - nextkey.GetValue()) < comp_level_integError
				)
				return false;
		}
	}
	
	
	return true;
	/*
	if (
		abs(deriv1 - deriv2) < 0.005 &&
		abs(deriv2 - deriv3) < 0.005 &&
		abs(deriv3 - deriv4) < 0.005 &&
		abs(deriv4 - derivl11) < 0.005 &&
		abs(derivl11 - derivr11) < 0.005 &&
		abs(derivr11 - derivl12) < 0.005 &&
		abs(derivl12 - derivr12) < 0.005 &&
		abs(derivr12 - derivl21) < 0.005 &&
		abs(derivl21 - derivr21) < 0.005 &&
		abs(derivr21 - derivl22) < 0.005 &&
		abs(derivl22 - derivr22) < 0.005
		)
	{
	}
	else
	{
		output2 << "keepinDeriv" << endl;
		return true;
	}
	output2 << "removable key" << endl;
	*/


	return false;
}

bool keepTestHorizon(  FbxAnimCurve* curve, FbxAnimCurveKey prevkey, FbxAnimCurveKey currkey, FbxAnimCurveKey nextkey)
{
	FbxTime prevt = prevkey.GetTime();
	FbxTime currt = currkey.GetTime();
	FbxTime nextt = nextkey.GetTime();

	bool needit = false;
	float prevEv;

	FbxLongLong tmpll = (currt - prevt).GetMilliSeconds() / 3;
	FbxTime termt;

	float tmpfs[6] = { -1, };
	FbxTime times[6];

	for (int termi = 0; termi < 3; termi++)
	{
		termt.SetMilliSeconds(tmpll*termi);
		times[termi] = prevt + termt;
	}

	tmpll = (nextt - currt).GetMilliSeconds() / 3;

	for (int termi = 0; termi < 3; termi++)
	{
		termt.SetMilliSeconds(tmpll*termi);
		times[3 + termi] = currt + termt;
	}

	output2 << setw(20 + output2.rdbuf()->in_avail() - 4096) << "timee : ";

	
	for (int i = 0; i < 6; i++)
	{
		
		output2 << setw(10) << setiosflags(ios::fixed) << setprecision(3) << (times[i]).GetSecondDouble() << "\t";
		tmpfs[i] = curve->Evaluate(times[i]);
	}

	output2 << endl << setw(20) << "value : ";
	for (int i = 0; i < 6; i++)
	{
		output2 << setw(10) << setiosflags(ios::fixed) << setprecision(3) << tmpfs[i] << "\t";
	}
	for (int i = 1; i<6;i++)
		if (abs(tmpfs[i] - tmpfs[i-1]) > comp_level_horizon)
		{
			output2 << "keepinHorizon at : " << i << endl;
			return true;
		}

	output2 << endl;
	return false;
}
int main(int argc, char **argv)
{
	const char* filename = "PC@GUN.fbx";

	
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
						output2 << "curve  " << ci << endl;
						
						vector<int> keys2Remove;
						for (int cki = 0; cki < keycnt; cki++)
						{
							FbxAnimCurveKey prevkey, currkey, nextkey;

							if (cki == 0 || cki == keycnt - 1)
								continue;
							
							currkey = curve->KeyGet(cki);
							prevkey = curve->KeyGet(cki-1);
							nextkey = curve->KeyGet(cki + 1);
							
							bool keepit;

							output2 << ci << "-" << cki;

//							keepit = keepTestHorizon(curve, prevkey, currkey, nextkey);
	//						if (keepit)
								keepit = slopkeepTest(curve, prevkey, currkey, nextkey);

							if (!keepit)
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
						
					}

				}
			}
			//이부분은 별로 효과없음
			/*
			for (int di = 0; di < nodes2del.size(); di++)
			{
				layer->RemoveMember(nodes2del[di]);
			}
			*/
			
		}
	}
	output << "cubic:linear:const  " << cubic << ":" << linear << ":" << cons << endl;
	FbxExporter* exporter = FbxExporter::Create(fm, "");
	const char* outFBXName = "PC@aaaaf.fbx";

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