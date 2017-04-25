#include "toonz/animationautocomplete.h"

#include "tcurves.h"
#include "drawutil.h"
#include "hungarian.h"

#include "toonz/hungarian.h"


/*
// hungarian method
//double AnimationAutoComplete::hungarian_method(double [], double [])
//{
  //  // for each row fint the min and subtract it foe each element in this row

  //double min_in_row =0;

  //for (int r =0 ; r < nrows ; r++)
  {
      min_in_row = C[r,0];
              for (int c =0; c <ncol ; c++)
      {
          if (C[r,c]< min_in_row )
          {
              min_in_row= c[r,c];
          }

                      for (int c=0 ; c< ncol; c++)
              {
                  C[r,c] -= min_in_row;
              }

      }
  }

  // find the zero in the resulting matrix , ithere is no starred * zero
  // in its row or column , satr z , repeat for each element in the matrix


     for (int r =0 ; r < nrows ; r++)
                  {

                      if (C[r,c] == 0 && RowCover [r] ==0 && ColCover[c]==0)

                      {
                          M[r,c]=1;
                              RowCover[r]=1;
                                  ColCover[c]=1;


                      }

                  }
                                  for (int r= 0; r< nrows ; r++)
                                  RowCover[r] = 0;
                                  for (int c=0;c< ncol;c++)
                                  Colcover[c]=0;





}


*/





//--------------------------------------------------------------------------------------
void AnimationAutoComplete::addStroke(TStroke* stroke)
{

	StrokeWithNeighbours* strokeWithNeighbours = new StrokeWithNeighbours();

	if(m_strokesWithNeighbours.size()!=0)
		m_strokesWithNeighbours.back()->nextStroke = strokeWithNeighbours;

	m_strokesWithNeighbours.push_back(strokeWithNeighbours);

	strokeWithNeighbours->stroke = stroke;

	int chuckCount = stroke->getChunkCount();
	for (int i = 0; i < chuckCount; i++)
	{
		std::vector<StrokeWithNeighbours*> neighbours = getNeighbours(stroke->getChunk(i));
		strokeWithNeighbours->neighbours.insert(neighbours.begin(), neighbours.end());
	}

	if (m_strokesWithNeighbours.size() >= 2)
		initializeSynthesis();
}

//--------------------------------------------------------------------------------------------------
double AnimationAutoComplete::gaussianConstant(SamplePoint chuck1, SamplePoint chuck2)
{
	TPointD sample1= chuck1->getP0();
	TPointD sample2= chuck2->getP0();
	TThickPoint s1; TThickPoint s2;
	s1.x=sample1.x;  s1.y=sample1.y;
	s2.x=sample2.x;  s2.y=sample2.y;
	double distance=tdistance2(s1,s2);
	distance=sqrt(distance);
	#define OMEGA 10
	return exp(-distance/OMEGA);
}

std::vector<StrokeWithNeighbours*> AnimationAutoComplete::getNeighbours(SamplePoint point)
{
	std::vector<StrokeWithNeighbours*> neighbours;

	for(int i = 0; i < m_strokesWithNeighbours.size(); i++)
	{
		TStroke* stroke = m_strokesWithNeighbours[i]->stroke;

		for(int j = 0; j < stroke->getChunkCount(); j++)
			if(withinSpaceVicinity(point, stroke->getChunk(j)))
			{

				neighbours.push_back(m_strokesWithNeighbours[i]);
				break;
			}
	}
	int size = m_strokesWithNeighbours.size();
	if(size>=2)
	{

	neighbours.push_back(m_strokesWithNeighbours[size-2]);
	neighbours.push_back(m_strokesWithNeighbours[size-3]);
	}

	return neighbours;
}

bool AnimationAutoComplete::withinSpaceVicinity(const SamplePoint samplePoint, const SamplePoint point)
{
	double distance = norm(samplePoint->getP2() - point->getP2());
	if(distance <= m_spaceVicinityRadius)
		return true;
	else
		return false;
}

void AnimationAutoComplete::initializeSynthesis()
{
	StrokeWithNeighbours* lastStroke = m_strokesWithNeighbours.back();
	std::vector<StrokeWithNeighbours*> similarStrokes = search(lastStroke);

	StrokeWithNeighbours* outputsStroke = assign(similarStrokes);

	if(outputsStroke)
		m_synthesizedStrokes.push_back(outputsStroke);
}

std::vector<StrokeWithNeighbours*> AnimationAutoComplete::getSynthesizedStrokes()
{
	return m_synthesizedStrokes;
}

double AnimationAutoComplete::pointsSimilarity(PointWithStroke* point1, PointWithStroke* point2)
{
	double dissimilarityFactor = 0;
	dissimilarityFactor += pow(getAppearanceSimilarity(point1, point2),2);
	dissimilarityFactor += pow(getTemporalSimilarity(point1, point2),2);
	dissimilarityFactor += pow(getSpatialSimilarity(point1, point2),2);

	return sqrt(dissimilarityFactor);
}

double AnimationAutoComplete::pointsSimilarityWithoutWeights(PointWithStroke *point1, PointWithStroke *point2)
{
    double dissimilarityFactor = 0;
    // the smaller the factor the more similar they are
    dissimilarityFactor += pow(getAppearanceSimilarity(point1, point2),2);
    dissimilarityFactor += pow(getTemporalSimilarity(point1, point2),2);
    dissimilarityFactor += pow(getSpatialSimilarity(point1, point2),2);

    return sqrt(dissimilarityFactor);
}

double AnimationAutoComplete::getAppearanceSimilarity(PointWithStroke* point1, PointWithStroke* point2)
{
	double dissimilarityFactor = 0;
	if(point1->stroke->getStyle() != point2->stroke->getStyle())
		dissimilarityFactor++;

	dissimilarityFactor += fabs(point1->point->getThickP0().thick - point2->point->getThickP0().thick);

	return dissimilarityFactor;
}

double AnimationAutoComplete::getTemporalSimilarity(PointWithStroke *point1, PointWithStroke *point2)
{
    double sampleId1=getSampleId(point1->index, point1->stroke->getChunkCount());
    double sampleId2=getSampleId(point2->index, point2->stroke->getChunkCount());
    return (fabs(sampleId1-sampleId2));


}

double AnimationAutoComplete::getSpatialSimilarity(PointWithStroke *point1, PointWithStroke *point2)
{
    TPointD direction1 = getNormal(point1);
    TPointD direction2 = getNormal(point2);
    return sqrt(pow(fabs(direction1.x-direction2.x),2)+pow(fabs(direction1.y-direction2.y),2));

}

std::vector<StrokeWithNeighbours*> AnimationAutoComplete::search(StrokeWithNeighbours* lastStroke)
{

    if(!lastStroke)
		return std::vector<StrokeWithNeighbours*>();

	double min = 10000000;
	StrokeWithScore score_stroke;
    std::vector<StrokeWithScore> scores;

    for(int i = m_strokesWithNeighbours.size()-2; i > m_strokesWithNeighbours.size()-30; i--)
	{
        double score = getNeighborhoodSimilarity(lastStroke, m_strokesWithNeighbours[i]);
        score_stroke.score = min;
        score_stroke.stroke = m_strokesWithNeighbours[i];
		if(score < min)
		{
			min = score;

		}
        scores.push_back(score_stroke);
	}

    std::vector<StrokeWithNeighbours*> similarstrokes;
    for(StrokeWithScore Score : scores)
	{
        double score = Score.score;

		if(score < 2 * min)
            similarstrokes.push_back(Score.stroke);
	}

    return similarstrokes;
}

StrokeWithNeighbours *AnimationAutoComplete::assign(std::vector<StrokeWithNeighbours *> similarStrokes)
{
	if(similarStrokes.empty())
		return nullptr;

	double min = 9999999999;
	StrokeWithScore outputStroke;
	StrokeWithNeighbours* lastStroke = m_strokesWithNeighbours.back();

	for (StrokeWithNeighbours* similarStroke : similarStrokes) {
		StrokeWithNeighbours* nextToSimilarStroke = similarStroke->nextStroke;
		StrokeWithNeighbours* nextStroke = generateSynthesizedStroke(lastStroke, similarStroke, nextToSimilarStroke);

        std::vector<SimilarPairPoints> similarStrokeMatchingPairs = getSimilarPairs(similarStroke, nextToSimilarStroke);
        std::vector<SimilarPairPoints> lastDrawnMatchingPairs = getSimilarPairs(nextStroke, lastStroke);

		for (int i = 0; i < similarStrokeMatchingPairs.size() || i < lastDrawnMatchingPairs.size(); i++)
		{
			double guassian = gaussianConstant(similarStrokeMatchingPairs[i].point1->point , similarStrokeMatchingPairs[i].point2->point);
			double similarityScore = pointsSimilarity(similarStrokeMatchingPairs[i].point1, similarStrokeMatchingPairs[i].point2);
			double similarityScoreWithoutWeights = pointsSimilarityWithoutWeights(lastDrawnMatchingPairs[i].point1, lastDrawnMatchingPairs[i].point2);
			double score = guassian * pow((similarityScore - similarityScoreWithoutWeights), 2);

			if (score > min)
				continue;

			min = score;
			outputStroke.score = min;
			outputStroke.stroke = similarStroke;
		}
	}

	return outputStroke.stroke;
}

std::vector<double> AnimationAutoComplete::differnceOfTwoNeighborhood(std::vector<SimilarPairPoints> similarPairPoints)
{
    for(SimilarPairPoints similarPair : similarPairPoints)
    {

    }
	std::vector<double> centralSimilarities1 = getCentralSimilarities(stroke1->stroke);
	std::vector<double> centralSimilarities2 = getCentralSimilarities(stroke2->stroke);

	  std::vector<double> centralSimilarities;


	  std::transform(centralSimilarities1.begin(),centralSimilarities1.end(),centralSimilarities2.begin(),std::back_inserter(centralSimilarities),
					 std::minus<double>());

	  std::transform(centralSimilarities.begin(), centralSimilarities.end(),centralSimilarities.begin(),centralSimilarities.begin(),std::multiplies<double>());

	  //remember to absolute

  return centralSimilarities;
}


double AnimationAutoComplete::operationsSimilarity(StrokeWithNeighbours* stroke1, StrokeWithNeighbours* stroke2)
{
	double similarityScore = 100000;
	double disSimilarityScore = 0;
    std::vector<SimilarPairPoints> similarPairs = getSimilarPairs(stroke1,stroke2);

  for(int i=0;i<similarPairs.size();i++)
  {
      disSimilarityScore += pointsSimilarity(similarPairs[i].point1,similarPairs[i].point2);
  }

  return disSimilarityScore/similarPairs.size();
}

double AnimationAutoComplete::getNeighborhoodSimilarity(StrokeWithNeighbours *stroke1, StrokeWithNeighbours *stroke2)
{
	//double neighboursSimilarity =operationsSimilarity(stroke1->stroke,stroke1->neighbours.->stroke);
    double neighbourhoodSimilarity = 0;

	for(StrokeWithNeighbours* neighbour : stroke1->neighbours)
        neighbourhoodSimilarity += operationsSimilarity(stroke1 ,neighbour);
		//std::transform(sum.begin(), sum.end(), neighboursSimilarity.begin(),
					   //neighboursSimilarity.begin(), std::plus<double>());

    return neighbourhoodSimilarity/stroke1->neighbours.size();
}

double AnimationAutoComplete::getSampleId(int index, int n)
{
    //in case we use global time stamp, we will multiply by 0.9
    // to avoid having samples in different strokes with the same global TS
    return (index/n);
}

std::vector<double> AnimationAutoComplete::getCentralSimilarities(std::vector<SimilarPairPoints> similarPairPoints)
{
    PointWithStroke* central1;
    PointWithStroke* central2;
    central1->stroke=similarPairPoints[0].point1.stroke;
    central2->stroke=similarPairPoints[0].point2.stroke;
    int n = similarPairPoints.size();
	std::vector<double> centralSimilarities;
    if(n%2==0)
	{
        central1->index=(n/2);
        central2->index=(n/2);
	}
	else
	{
        central1->index=((n/2)+1);
        central2->index=((n/2)+1);
	}
    for(int i=0;i<n;i++)
	{
		if (i!=central->index)
		{
			PointWithStroke* point;
			point->index=i;
			point->stroke=stroke;

			double similarityfactor = pointsSimilarity(point, central);
			centralSimilarities.push_back(similarityfactor);
		}
	}
	return centralSimilarities;

}

double AnimationAutoComplete::magnitude(std::vector<double> points)
{
	double sum=0;
	for(int i=0;i<points.size();i++)
	{
		sum+= points[i]*points[i];
	}
	sum = sqrt(sum);
	return sum;
}

std::vector<SimilarPairPoints> AnimationAutoComplete::getSimilarPairs(StrokeWithNeighbours *, StrokeWithNeighbours *)
{

}


StrokeWithNeighbours *AnimationAutoComplete::generateSynthesizedStroke(StrokeWithNeighbours *lastStroke, StrokeWithNeighbours *similarStroke, StrokeWithNeighbours *nextToSimilarStroke)
{
	StrokeWithNeighbours* outputStroke = new StrokeWithNeighbours();
	std::vector<TThickPoint> points;

	int loopCount = (int)fmin((double)similarStroke->stroke->getChunkCount(), (double)nextToSimilarStroke->stroke->getChunkCount());

	for(int i = 0; i < loopCount; i++)
	{
		TPointD p1 = similarStroke->stroke->getChunk(i)->getP0();
		TPointD p2 = nextToSimilarStroke->stroke->getChunk(i)->getP0();
		double x1 = p1.x , x2 = p2.x;
		double y1 = p1.y , y2 = p2.y;
		double x_diffrence = x2-x1;
		double y_diffrence = y2-y1;
		double x_output = lastStroke->stroke->getChunk(i)->getP0().x + x_diffrence;
		double y_output = lastStroke->stroke->getChunk(i)->getP0().y + y_diffrence;

		TThickPoint p = TThickPoint(x_output,y_output,similarStroke->stroke->getChunk(i)->getThickP0().thick);

		if(points.size() > 0)
		{
			TThickPoint old = points.back();
			if (norm2(p - old) < 4) continue;
			TThickPoint mid((old + p) * 0.5, (p.thick + old.thick) * 0.5);
			points.push_back(mid);
		}
		points.push_back(p);
	}

	outputStroke->stroke = new TStroke(points);
	return outputStroke;
}

std::vector<SimilarPairStrokes *> AnimationAutoComplete::getNeighborhoodMatchingPairs(StrokeWithNeighbours *stroke1, StrokeWithNeighbours *stroke2)
{
      HungarianAlgorithm hA;
      vector<int> Assignment;
      hA.Solve()
}

TPointD AnimationAutoComplete::meanGlobal(std::vector<SamplePoint> globalSamples)
{
	double mean1 =0;
	double mean2 =0;
   for(SamplePoint sample : globalSamples)
   {
	   mean1+=sample->getP0().x;
	   mean2+=sample->getP0().y;

   }
   mean1 /=globalSamples.size();
   mean2/=globalSamples.size();
   TPointD mean;
   mean.x=mean1;
   mean.y=mean2;
   return mean;
}

std::vector<StrokeWithScore> AnimationAutoComplete::getSimilarStrokes(StrokeWithNeighbours* stroke)
{
	std::vector<StrokeWithScore> similarStrokes;

	for(int i = 0; i < m_strokesWithNeighbours.size(); i++)
	{
		StrokeWithScore strokeWithScore;
		strokeWithScore.stroke = m_strokesWithNeighbours[i];
		strokeWithScore.score = operationsSimilarity(stroke, m_strokesWithNeighbours[i]);
		similarStrokes.push_back(strokeWithScore);
	}

	return similarStrokes;
}

SimilarPairPoints* AnimationAutoComplete::meanLocal(std::vector<SimilarPairPoints *> localPairs)
{
	double mean1x = 0;
	double mean1y = 0;
	double mean2x = 0;
	double mean2y = 0;

    for(SimilarPairPoints* similarPair : localPairs)
	{
		int index1 = similarPair->point1->index;
		mean1x +=  similarPair->point1->stroke->getChunk(index1)->getP0().x;
		mean1y+= similarPair->point1->stroke->getChunk(index1)->getP0().y;
		int index2 = similarPair->point2->index;
		mean2x = similarPair->point2->stroke->getChunk(index2)->getP0().x;
		mean2y = similarPair->point2->stroke->getChunk(index2)->getP0().y;

	}
    SimilarPairPoints* mean = new SimilarPairPoints();
   int size = localPairs.size();
	mean->point1->point->setP0(TPointD(mean1x/size,mean1y/size));
	mean->point2->point->setP0(TPointD(mean2x/size,mean2y/size));

	return mean;

}

TPointD AnimationAutoComplete::deviationGlobal(std::vector<SamplePoint> globalSamples)
{
	TPointD mean = meanGlobal(globalSamples);
	double stdX = 0;
	double stdY = 0;
	TPointD std;
	int size = globalSamples.size()-1;

	for(SamplePoint samplePoint: globalSamples)
	{
		stdX+=pow(samplePoint->getP0().x-mean.x,2);
		stdY+=pow(samplePoint->getP0().y-mean.y,2);
	}

	std.x=stdX/size;
	std.y=stdY/size;
	return std;
}

SimilarPairPoints* AnimationAutoComplete::deviationLocal(std::vector<SimilarPairPoints *> localPairs)
{
    SimilarPairPoints* mean = meanLocal(localPairs);
	double std1X =  0;
	double std1Y =  0;
	double std2X =  0;
	double std2Y =  0;
    SimilarPairPoints* std = new SimilarPairPoints();

	int size = localPairs.size()-1;


    for(SimilarPairPoints* similarPair: localPairs)
	{
		int index1 = similarPair->point1->index;
		std1X+=pow(similarPair->point1->stroke->getChunk(index1)->getP0().x-mean->point1->point->getP0().x,2);
		std1Y+=pow(similarPair->point1->stroke->getChunk(index1)->getP0().y-mean->point1->point->getP0().y,2);
		int index2 = similarPair->point2->index;
		std2X+=pow(similarPair->point2->stroke->getChunk(index2)->getP0().x-mean->point2->point->getP0().x,2);
		std2Y+=pow(similarPair->point2->stroke->getChunk(index2)->getP0().y-mean->point2->point->getP0().y,2);
	}
	std->point1->point->setP0(TPointD(std1X/size,std1Y/size));
	std->point2->point->setP0(TPointD(std2X/size,std2Y/size));
	return std;
}

SimilarPairPoints AnimationAutoComplete::getMostSimilarPoint(PointWithStroke *point, TStroke *stroke)
{
    SimilarPairPoints mostSimilarPair;

	mostSimilarPair.point1 = point;
	PointWithStroke* point2 = new PointWithStroke(stroke->getChunk(0), stroke);
	mostSimilarPair.dissimilarityFactor = pointsSimilarity(point, point2);
	mostSimilarPair.point2 = point2;

	for (int i = 1; i < stroke->getChunkCount(); i++)
	{
		point2 = new PointWithStroke(stroke->getChunk(i), stroke);
		double similarityScore = pointsSimilarity(point, point2);
		if (mostSimilarPair.dissimilarityFactor > similarityScore)
		{
			delete mostSimilarPair.point2;
			mostSimilarPair.point2 = point2;
			mostSimilarPair.dissimilarityFactor = similarityScore;
		}
		else delete point2;
	}

    return mostSimilarPair;
}

SimilarPairStrokes ** AnimationAutoComplete::prepareHungarian(StrokeWithNeighbours *stroke1, StrokeWithNeighbours *stroke2)

{
    SimilarPairStrokes** HungarianMatrix;
    std::vector<std::vector<double>> Scores;
    std::vector<std::pair<TStroke,std::vector<TStroke>> correspondingStrokes;
    HungarianMatrix= new SimilarPairStrokes *[stroke2->neighbours.size()];
    int i=0;
    int j=0;
    for(StrokeWithNeighbours* sN1 : stroke1->neighbours)
    {
        HungarianMatrix[i]= new SimilarPairStrokes [stroke1->neighbours.size()];
        for(StrokeWithNeighbours* sN2 : stroke2->neighbours)
        {
            SimilarPairStrokes hungarian ;

            hungarian.score = operationsSimilarity(sN1, sN2);
            hungarian.stroke1=sN1->stroke;
            hungarian.stroke2=sN2->stroke;

            HungarianMatrix[i][j]=hungarian;
            j++;
        }
        i++;
        j=0;
    }
    return HungarianMatrix;
}
//TODO: remove at production
std::vector<TStroke*> AnimationAutoComplete::drawSpaceVicinity(TStroke *stroke)
{
	std::vector<TStroke*> strokes;

	for(int i = 0; i < stroke->getChunkCount(); i++)
		strokes.push_back(makeEllipticStroke(3, stroke->getChunk(i)->getP0(), m_spaceVicinityRadius, m_spaceVicinityRadius));
    return strokes;
}

HungerianMatrix AnimationAutoComplete::setHungerianMatrix(StrokeWithNeighbours *stroke1, StrokeWithNeighbours *stroke2)
{
    HungarianAlgorithm obj_hungarianAlgorithm;
    std::vector<StrokeWithNeighbours*>ReorderStroke1;
    std::vector<StrokeWithNeighbours*>ReorderStroke2;
    std::vector<int>SolveFunctionVector;

    HungerianMatrix m_hungerianMatrix;

    for(StrokeWithNeighbours*stroke:stroke1->neighbours)
    {
        ReorderStroke1.push_back(stroke);
    }
    for(StrokeWithNeighbours*stroke:stroke2->neighbours)
    {
        ReorderStroke2.push_back(stroke);
    }
        for(int i = 0; i<ReorderStroke1.size(); i++)
        {
            std::vector<double>PushedVector;
            for(int j=0;j<ReorderStroke2.size();j++)
            {

     double Score=operationsSimilarity(ReorderStroke1[i],ReorderStroke2[j]);
        PushedVector.push_back(Score);
        }
         m_hungerianMatrix.push_back(PushedVector);
    }
 obj_hungarianAlgorithm.Solve(m_hungerianMatrix ,SolveFunctionVector);
}

TStroke* AnimationAutoComplete::drawstrokeLine(TStroke *stroke)
{
	//todo: strokeline
    std::vector<TPointD> vec;
    if( 1>=stroke->getChunkCount())
      {  return nullptr;}
    else {
    vec.push_back(stroke->getChunk(1)->getP0());

    double y2 = stroke->getChunk(1)->getP1().y;
    double y1 = stroke->getChunk(1)->getP0().y;
    double x2 = stroke->getChunk(1)->getP1().x;
    double x1 = stroke->getChunk(1)->getP0().x;

    double slope_tangent= (y2 - y1) / (x2 - x1);
   if (slope_tangent==0)
   {
       double new_x=x1+100;
       vec.push_back(TPointD(new_x,y1));
       TStroke* strokeLine = new TStroke(vec);
       return strokeLine;
   }

   else {
    double c_tangent = y2 - (slope_tangent * x2);

    double slope_prependicular=1/slope_tangent;
    double c_prependicular=y2+(slope_prependicular*x2);

    double new_x = x2 + 100;
    double new_y =(-slope_prependicular*new_x )+c_prependicular;

    vec.push_back(TPointD(new_x,new_y));
	TStroke* strokeLine = new TStroke(vec);
    return strokeLine;}
    }
}



//TODO : getNormal
TPointD AnimationAutoComplete::getNormal(PointWithStroke* pointer)
{
    if( pointer->index>=pointer->stroke->getChunkCount())
        return ;
    else {
    //vec.push_back(stroke->getChunk(pointer->index)->getP0());

    double y2 = pointer->stroke->getChunk(pointer->index)->getP1().y;
    double y1 = pointer->stroke->getChunk(pointer->index)->getP0().y;
    double x2 = pointer->stroke->getChunk(pointer->index)->getP1().x;
    double x1 = pointer->stroke->getChunk(pointer->index)->getP0().x;

    double slope_tangent= (y2 - y1) / (x2 - x1);
    if(slope_tangent==0)
    {
        return TPointD(0,1);

    }
    else{
        double c_tangent = y2 - (slope_tangent * x2);

        double slope_prependicular=1/slope_tangent;
        double c_prependicular=y2+(slope_prependicular*x2);

    double new_x = x2 + 10;
    double new_y =(-slope_prependicular*new_x )+c_prependicular;
    // Todo:Magnitude sqrt() lel x pow 2 + y pow 2 ;
    double x_def=new_x-x2;
    double y_def=new_y-y2;
    double x_pow2=x_def*x_def;
    double y_pow2= y_def*y_def;
    double point_magnitude=sqrt(x_pow2+y_pow2);
    double x_unit=x_def/point_magnitude;
    double y_unit =y_def/point_magnitude;
    return TPointD(x_unit,y_unit);}}

}

void GlobalSimilarityGraph::insertNode(SimilarPairPoints *pair, std::vector<SimilarPairPoints *> connections)
{
    this->connections.insert(std::pair<SimilarPairPoints*, std::vector<SimilarPairPoints*>>(pair, connections));
	numberOfNodes++;
}

std::vector<SimilarPairPoints *> GlobalSimilarityGraph::getConnections(SimilarPairPoints *pair)
{
	if (pair)
		return this->connections[pair];
    return std::vector<SimilarPairPoints *>();
}

void AnimationAutoComplete::getSimilarPairPoints(StrokeWithNeighbours *stroke1, StrokeWithNeighbours *stroke2)
{
    // TODO; replace el harie with HunharianMatrix
    std::vector<std::vector<double>> input;
    std::vector<int>assignment;
    int sizeSTroke1 = stroke1->stroke->getChunkCount();
    int sizeSTroke2 = stroke2->stroke->getChunkCount();
    std::vector<SimilarPair>similarPointsIndex;

    for(int i=0;i<sizeSTroke1;i++)
    {
       for(int j=0;j<sizeSTroke2;j++)
       {
          double pointsimilar = pointsSimilarity(stroke1->stroke(i),stroke2->stroke(j));
          input[i][j]=pointsimilar;
       }
    }

    HungarianAlgorithm hungarian;

    hungarian.Solve(input,assignment);

    for(int i=0;i<assignmet->size();i++)
    {
       if(assignment[i]!=-1)
       {
          SimilarPair p;
          PointWithStroke point1;

          point1.index=i;
          PointWithStroke point2;
          point2.index=assignment[i];
          p.point1=point1;
          p.point2=point2;

        similarPointsIndex.push_back(p);
       }
    }
}





