#include <string>
#include <cmath>
#include <memory>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <math.h>

#include <itkSpatialObjectReader.h>
#include <itkSpatialObjectWriter.h>

#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkSmartPointer.h>
#include <vtkCell.h>
#include <vtkFloatArray.h>

#include "dtitypes.h"
#include "regression.h"
#define pi 3.1415926535

regression::regression()
{
}
regression::~regression()
{
}

void regression::regression_main(std::string output_file, std::string param_str, std::vector< std::vector<double> > length, double step_size, double bandwidth, int stat, int noise_model, double q_perc, int all_flag, bool windowOn, int window, std::string output_viz_file,bool worldspace)
{
  ofstream fp_output_file;
  //sorting the list based on arc length
  int length_size = length.size();
  std::cout<<"size of length is "<<length_size;
  std::sort(length.begin(), length.end(), sortFunction ) ;

  //Writing header information to output file
  if (all_flag==-1 || all_flag==1)
  {
    fp_output_file.open(output_file.c_str(),ios::app);
    if (noise_model == 1 && stat!=1)
      fp_output_file<<"Noise Model: Beta\t";
    else if (noise_model == 1 && stat==1)
      fp_output_file<<"Noise Model: Gaussian\t";
    else
      fp_output_file<<"Noise Model: Gaussian\t";
	
    if (stat==1)
      fp_output_file<<"Statistics: Quantile (quantile percent: "<<q_perc<<")\n";
    else if (stat ==3)
      fp_output_file<<"Statistics: Mode\n";
    else
      fp_output_file<<"Statistics: Mean\n";
	
    fp_output_file<<"Arc Length parametrization (Step size): "<<step_size<<"\tStandard Deviation for kernel window: "<<bandwidth<<"\n";
    fp_output_file<<"Parameter chosen for regression: "<<param_str<<"\t\t";
    if (worldspace)
      fp_output_file<<"World space chosen\n";
    else
      fp_output_file<<"Image space chosen\n";
    
    fp_output_file.close();
  }

  //Calling the appropriate regression function
  if (all_flag == -1)
  {
    if (noise_model == 1 && stat!=1)	
    {
      //Calling Regression_Beta_Gaussian function	for Beta (1)
      //DEBUG
      //std::cout<<"writing result for the parameter "<<param_str<<std::endl;
      Regression_Beta_Gaussian(output_file, length, step_size, bandwidth, noise_model, stat, all_flag, windowOn, window, output_viz_file);
      cout<<"REGRESSION FUNCTION CALLED WITH BETA NOISE MODEL: Regression_Beta_Gaussian, stat (Quantile,Mean,Mode):"<<stat<< endl;
    }
    else if (noise_model == 2 && stat!=1)
    {
      //Calling Regression_Beta_Gaussian function	for Gaussian (2)
      //DEBUG
      //std::cout<<"writing result for the parameter "<<param_str<<std::endl;
      Regression_Beta_Gaussian(output_file, length, step_size, bandwidth, noise_model, stat, all_flag, windowOn, window, output_viz_file);
      cout<<"REGRESSION FUNCTION CALLED WITH GAUSSIAN NOISE MODEL: Regression_Beta_Gaussian, stat(Quantile,Mean,Mode):"<< stat<<endl;
    }
    else if (stat == 1)
    {	
      //Calling Regression_Quantile function	
      Regression_Quantile(output_file, length, step_size, bandwidth, q_perc, all_flag);
      cout<<"REGRESSION FUNCTION CALLED, with Gaussian weights : Regression_Quantile, q_perc:"<<q_perc<< endl;
    }
    else	
    {
      cout<<"Invalid combination of noise model and statistics provided as input\n Exiting without generating results\n";
      exit(0);
    }
  }
  else
  {
    Regression_Beta_Gaussian(output_file, length, step_size, bandwidth, 2, 2, all_flag, windowOn, window, output_viz_file);
    cout<<"REGRESSION FUNCTION CALLED WITH GAUSSIAN NOISE MODEL, Stat: Mean, for Param # : "<<all_flag<<endl;
  }
}

bool regression::sortFunction(std::vector< double > i , std::vector< double > j )
{
  return ( i[ 0 ] < j[ 0 ] ) ;
}


void regression::Regression_Quantile(std::string output_file, std::vector< std::vector<double> > length, double step_size, double bandwidth, double q_perc, int all_flag)
{
  //Variables
  ofstream fp_output_file;
	
  //create an array with length equally dividing the min and max of length array
  int l_counter = length.size();
  double min_length = length[0][0]; 
  double max_length=length[l_counter-1][0];			//since length[][] is sorted 
  int reg_counter = ceil((max_length - min_length)/step_size);	//note: min_length is negative. We want the last sampled point to be before the last actual fiber point
  double** regression_result_quan=new double*[reg_counter];			//will hold the arc length and result of the quantile 
  for(int x=0; x<reg_counter; x++)
	  regression_result_quan[x]=new double[2];

  for (int k=0;k<reg_counter;k++)
  {
    regression_result_quan[k][0]= min_length+k*step_size;		//initializing the length
  }

  //finding quantile for each window
  for (int i=0; i<reg_counter; i++)
  {	
    //create list of param values in the current window
    //holds param value and its weight in that window
	double** curr_param=new double*[l_counter]; 
    for(int x=0; x<l_counter; x++)
	  curr_param[x]=new double[2];
    
    double range_min= regression_result_quan[i][0] -bandwidth*1;		//***************    taking cut off as 1 std deviation
    double range_max= regression_result_quan[i][0] +bandwidth*1;
    double* current_weights=new double[l_counter];
	double* y=new double[l_counter];
    for (int k=0;k<l_counter;k++)
    {
      y[k] = (regression_result_quan[i][0]-length[k][0])/bandwidth;
      current_weights[k] = (exp(-(y[k]*y[k])/2))/(sqrt(2*pi)*bandwidth);
    }
    int curr_param_index=0;
    for (int j=0;j<l_counter;j++)
    {
      {
	if (length[j][0] >=range_min && length[j][0] <=range_max)						
	{
	  //need to store this diffusion param value and its weight
	  curr_param[curr_param_index][0]= length[j][1];
	  curr_param[curr_param_index][1]= current_weights[j];
	  curr_param_index++;
	}
      }
    }
    //now we ve all param values and their weights in curr_param for the current window
    //now sort them on param
    for (int f=0; f<curr_param_index-1; f++) 
    {
      for (int g=0; g<curr_param_index-1-f; g++)
      {
	if (curr_param[g+1][0] < curr_param[g][0]) 
	{  /* compare the two neighbors */
	  double tmp_param = curr_param[g][0];         /* swap a[j] and a[j+1]      */
	  double tmp_wt = curr_param[g][1];
	  curr_param[g][0] = curr_param[g+1][0];
	  curr_param[g][1] = curr_param[g+1][1];
	  curr_param[g+1][0] = tmp_param;
	  curr_param[g+1][1] = tmp_wt;
	}
      }
    }
    
    double q_step;
    int q_bin=20;
    q_step = (curr_param[curr_param_index-1][0] - curr_param[0][0] ) /  q_bin;
	double** q_pdf=new double*[q_bin]; //0 holds the bin value for pdf, 1 holds the no of points contributing to this bin
    for(int x=0; x<q_bin; x++)
	  q_pdf[x]=new double[2];
    double total =0;
    
    for (int g=0; g<q_bin;g++)
    {
      //initialize bin values
      q_pdf[g][0]=0;q_pdf[g][1]=0;
      for (int h=0; h<curr_param_index ; h++)
      {
	double curr_bin_low= curr_param[0][0] + q_step * g;
	if (curr_param[h][0] >= curr_bin_low && curr_param[h][0] < (curr_bin_low + q_step))
	{
	  q_pdf[g][0] += (1 * curr_param[h][1]);
	  q_pdf[g][1] ++;
	  total += (1 * curr_param[h][1]);
	}
      }
    }
    
    //now create cdf
    double* q_cdf=new double[q_bin];
    q_cdf[0] = q_pdf[0][0]/total;
    for (int g=1; g<q_bin; g++)
    {
      q_cdf[g] = (q_cdf[g-1] + q_pdf[g][0]/total);
    }
    //now find the actual quantile....
    double curr_quan_bin_val = q_perc/100 * q_cdf[q_bin-1];
    double curr_quan = 0;
    for (int g=0; g<q_bin -1 ;g++)
    {
      if (curr_quan_bin_val > q_cdf[g] && curr_quan_bin_val <= q_cdf[g+1])
      {
	double curr_bin_low= curr_param[0][0] + q_step * g;
	curr_quan = curr_bin_low + q_step;	//right of curr bin or lower of next bin
      }
    }
    regression_result_quan[i][1] = curr_quan;
  }
  
  //finding the standard deviation now using the fitted value per window....

  double* std_dev=new double[reg_counter];
  int* num_fib_points=new int[reg_counter];
  for (int i=0; i<reg_counter; i++)
  {	
    double range_min= regression_result_quan[i][0] -bandwidth*1;		//***************    taking cut off as 1 std deviation
    double range_max= regression_result_quan[i][0] +bandwidth*1;
    
    double temp_sq_sum = 0.0;
    int num_points=0;
    for (int j=0;j<l_counter;j++)
    {
      if (length[j][0] >=range_min && length[j][0] <=range_max)						
      {
	temp_sq_sum+= (length[j][1]-regression_result_quan[i][1]) * (length[j][1]-regression_result_quan[i][1]);
	num_points++;
      }
    }
    std_dev[i]=sqrt(temp_sq_sum/num_points);
    num_fib_points[i] = num_points;
  }
  
  //Writing to output files
  
  //Writing the given parameter along with std deviation, using the given noise model and MLE
  if (all_flag==-1)
  {
   
    fp_output_file.open(output_file.c_str(),ios::app);
    fp_output_file<<"Number of samples along the bundle: "<<reg_counter<<"\n";
    fp_output_file<<"Arc_Length,#_fiber_points,Parameter_Value,Std_Dev,Param+Std_Dev,Param-Std_Dev\n";
    for (int i=0; i<reg_counter; i++)
    {	
      fp_output_file<<regression_result_quan[i][0]<<","<<num_fib_points[i]<<","<<regression_result_quan[i][1]<<","<<std_dev[i]<<","<<(regression_result_quan[i][1]+ std_dev[i])<<","<<(regression_result_quan[i][1]- std_dev[i])<<"\n";
    }
  }
}

void regression::Regression_Beta_Gaussian(std::string output_file, std::vector< std::vector<double> > length, double step_size, double bandwidth, int noise_model, int stat, int all_flag, bool windowOn, int window, std::string output_viz_file)
{
  //create an array with length equally dividing the min and max of length array

  int l_counter = length.size();
  double min_length = length[0][0]; 
  double max_length=length[l_counter-1][0];	//since length[][] is sorted 
  int reg_counter = ceil((max_length - min_length )/step_size );	//note: min_length is negative. We want the last sampled point to be before the last actual fiber point

  //getting minimum and maximum of the parameter for Beta regression
  double min= find_min(length, l_counter);
  double max= find_max(length, l_counter);

  //Variables
  double temp_param = 0.0, weight_sum = 0.0, V2 =0.0, clamp_count=0;
 double** beta_regression_result=new double*[reg_counter]; //will hold the result of the regression for Beta and Gaussian
    for(int x=0; x<reg_counter; x++)
	  beta_regression_result[x]=new double[4];
  double** std_dev=new double*[reg_counter]; 
    for(int x=0; x<reg_counter; x++)
	  std_dev[x]=new double[10];
  int* num_fib_points=new int[reg_counter];
  double** hist=new double*[reg_counter];				//histogram has 50 bins
    for(int x=0; x<reg_counter; x++)
	  hist[x]=new double[50];
  double mode_est=0, mean_beta=0, wt_sample_mean=0;
  ofstream fp_output_file;
  //initializing 
  for (int k=0;k<reg_counter;k++)
  {
    beta_regression_result[k][0] = min_length+k*step_size;		
    std_dev[k][0] = min_length+k*step_size;
    num_fib_points[k] = 0;
    for (int l=0;l<50;l++)
      hist[k][l]=0;
  }
  for (int i=0; i<reg_counter; i++)
  {	
    double range_min= beta_regression_result[i][0] -bandwidth*1;		//***************    taking cut off as 1 std deviation
    double range_max= beta_regression_result[i][0] +bandwidth*1;
    double* current_weights=new double[l_counter];
	double* y=new double[l_counter];
    for (int k=0;k<l_counter;k++)
    {
      y[k] = (beta_regression_result[i][0]-length[k][0])/bandwidth; //distance
      //gaussian kernel
      current_weights[k] = (exp(-(y[k]*y[k]/2))/(bandwidth * sqrt(2*pi)));
    }
    
    
    //if l is in current window, we find weighted mean and variance for diffusion parameter value using parameter estimation
    temp_param = 0.0, weight_sum = 0.0, V2 =0.0;
    //weighted sample mean
    for (int j=0;j<l_counter;j++)
    {
      if (length[j][0] >=range_min && length[j][0] <=range_max)						
      {
	weight_sum += current_weights[j];	
	temp_param += (current_weights[j] * length[j][1]);

      }
      
    }
    if (weight_sum!=0)
      wt_sample_mean = temp_param / weight_sum;
    else
      wt_sample_mean = 0;
    
    //normalizing the weights
    for (int j=0;j<l_counter;j++)
    {
      if (length[j][0] >=range_min && length[j][0] <=range_max)						
      {
	current_weights[j] /= weight_sum;	
      }
    }
    
    //Creating histogram for window using NORMALIZED WEIGHTS
    double hist_sum =0; 			//hist_step=0.02;
    double hist_step=(max-min)/50;
    for (int j=0;j<l_counter;j++)
    {
      if (length[j][0] >=range_min && length[j][0] <=range_max)						
      {
	for (int k=0; k<50 ; k++)
	{	
	  double bin_low= min + (k*hist_step);
	  double bin_high= bin_low + hist_step;
	  if (length[j][1]>= bin_low && length[j][1]< bin_high)
	  {
	    hist[i][k] += current_weights[j];
	    hist_sum+= current_weights[j];
	    
	  }	
	}
	//If window visualization needed, then writing the histogram values to output_viz file
	if (windowOn && window==i && window>=0 && window<reg_counter)
	  {
	    ofstream fp_output_viz_file;
	    fp_output_viz_file.open(output_viz_file.c_str(),ios::out);
	    fp_output_viz_file<<"kernel window's distribution histogram (Window # chosen: "<<window<<" from a total of "<<reg_counter<<" windows\n";
	    fp_output_viz_file<<"Parameter-lower bin value"<<","<<"normalized weights"<<endl;
	    for (int k=0;k<50;k++)
	      {
		fp_output_viz_file<<(min + (k*hist_step))<<","<<hist[i][k]<<endl;
	      }
	    fp_output_viz_file.close();
	  }
      }
    }


    //now finding weighted sample variance
    //using UNBIASED ESTIMATOR for sample variance of weighted samples
    double temp_param_var = 0.0;
    for (int j=0;j<l_counter;j++)
    {
      if (length[j][0] >=range_min && length[j][0] <=range_max)						
      {
	V2 += (current_weights[j]* current_weights[j]);
	temp_param_var += (current_weights[j] * (pow((length[j][1] - wt_sample_mean),2)) );	
      }
    }
    double wt_sample_var;
    if ((1-V2) != 0)
      wt_sample_var = temp_param_var / (1 - V2);
    else
    {
      wt_sample_var = 0;
      cout<<"Found V2 =0 : setting weighted sample variance = 0 for this kernel window\n";
    }
    
    //accounting for a range other than [0,1] for beta distribution
    //wt_sample_mean = (wt_sample_mean - min)/ (max - min);		
    //wt_sample_var = wt_sample_var / (pow((max-min),2));
    
    //now we find estimates of alpha and beta usings sample mean and var
    double alpha_est, beta_est;
    if (wt_sample_var != 0)
    {
      alpha_est= wt_sample_mean * ( ( (wt_sample_mean * (1-wt_sample_mean)) / wt_sample_var) - 1);
      beta_est= (1-wt_sample_mean) * ( ( (wt_sample_mean * (1-wt_sample_mean)) / wt_sample_var) - 1);
    }
    else
    {
      alpha_est = -1;
      beta_est = -1;
    }
    
    if (alpha_est <2 )		
      {
	clamp_count++;
	alpha_est=2.0001;
      }
    if (beta_est <2)
    {
      clamp_count++;
      beta_est=2.0001;
    }
    
    //now finding MLE by finding mode using alpha, beta
    if (wt_sample_var != 0)
    {
      mode_est = (alpha_est - 1) / (alpha_est + beta_est - 2);
      mean_beta = alpha_est / (alpha_est + beta_est);
    }
    else 
    {	
      //making both equal to weighted sample mean of the distribution
      //cout<<"Weighted sample variance=0 for this kernel window:\n\tsetting maximum likelihood estimate for this window=weighted sample mean\n";
      mode_est = wt_sample_mean;
      mean_beta = wt_sample_mean;
    }
    
    beta_regression_result[i][1] = mode_est;
    beta_regression_result[i][2] = wt_sample_mean;
    beta_regression_result[i][3] = mean_beta;
  }
  
  //finding the standard deviation now using the fitted value per window....
  
  for (int i=0; i<reg_counter; i++)
  {	
    double range_min= beta_regression_result[i][0] -bandwidth*1;		//***************    taking cut off as 1 std deviation
    double range_max= beta_regression_result[i][0] +bandwidth*1;
    
    double temp_sq_sum = 0.0, temp_sq_sum_gauss=0.0, temp_sq_sum_beta_mean=0.0;
    int num_points=0;
    for (int j=0;j<l_counter;j++)
    {
      if (length[j][0] >=range_min && length[j][0] <=range_max)						
      {
	temp_sq_sum+= (length[j][1]-beta_regression_result[i][1]) * (length[j][1]-beta_regression_result[i][1]);
	temp_sq_sum_gauss+= (length[j][1]-beta_regression_result[i][2]) * (length[j][1]-beta_regression_result[i][2]);
	temp_sq_sum_beta_mean+= (length[j][1]-beta_regression_result[i][3]) * (length[j][1]-beta_regression_result[i][3]);
	
	num_points++;
      }
    }
    
    if (num_points > 1)
    {
      std_dev[i][1]=sqrt(temp_sq_sum/num_points);
      std_dev[i][4]=sqrt(temp_sq_sum_gauss/num_points);
      std_dev[i][7]=sqrt(temp_sq_sum_beta_mean/num_points);
    }
    else
    {
      std_dev[i][1]=0;
      std_dev[i][4]=0;
      std_dev[i][7]=0;
    }
    
    num_fib_points[i] = num_points;
    
    std_dev[i][2]= beta_regression_result[i][1]+ std_dev[i][1];
    std_dev[i][3]= beta_regression_result[i][1]- std_dev[i][1];
    std_dev[i][5]= beta_regression_result[i][2]+ std_dev[i][4];
    std_dev[i][6]= beta_regression_result[i][2]- std_dev[i][4];
    std_dev[i][8]= beta_regression_result[i][3]+ std_dev[i][7];
    std_dev[i][9]= beta_regression_result[i][3]- std_dev[i][7];
  }
  
  //writing to output files
  
  //Writing the given parameter along with std deviation, using the given noise model and MLE
  if (all_flag==-1)
  {
    if (noise_model==1)
      cout<<"Number of kernel windows where alpha and beta estimates were clamped to 2.0001 to avoid symmetric, strictly concave or convex distributions : "<<clamp_count<<endl;

    fp_output_file.open(output_file.c_str(),ios::app);
    fp_output_file<<"Number of samples along the bundle: "<<reg_counter<<"\n";
    fp_output_file<<"Arc_Length,#_fiber_points,Parameter_Value,Std_Dev,Param+Std_Dev,Param-Std_Dev\n";
    
    for (int i=0; i<reg_counter; i++)
    {	
      if (noise_model==1 && stat==3)
	{
	  fp_output_file<<beta_regression_result[i][0]<<","<<num_fib_points[i]<<","<<beta_regression_result[i][1]<<","<<std_dev[i][1]<<","<<std_dev[i][2]<<","<<std_dev[i][3]<<"\n";
	}
      else if (noise_model==1 && stat==2)
	{
	  fp_output_file<<beta_regression_result[i][0]<<","<<num_fib_points[i]<<","<<beta_regression_result[i][3]<<","<<std_dev[i][7]<<","<<std_dev[i][8]<<","<<std_dev[i][9]<<"\n";
	}
      else if (noise_model==2 && (stat==2 || stat==3))	//For Gaussian, mean and mode are the same
	{
	  fp_output_file<<beta_regression_result[i][0]<<","<<num_fib_points[i]<<","<<beta_regression_result[i][2]<<","<<std_dev[i][4]<<","<<std_dev[i][5]<<","<<std_dev[i][6]<<"\n";
	}
    }
    fp_output_file.close();
  }
  else
  {
    //writing current parameter result (with Gaussian Mean) to the vector all_results
    for ( int i=0; i<reg_counter; i++)
      {
	if (all_flag==1)
	  {
	    all_results.push_back(std::vector<double>());
	    all_results[i].push_back(beta_regression_result[i][0]);
	    all_results[i].push_back(num_fib_points[i]);
	  }
	all_results[i].push_back(beta_regression_result[i][2]);
      }
  }
  
}

std::vector< std::vector<double> > regression::get_all_results()
{
  return all_results;
}

double regression::find_min(std::vector< std::vector<double> > length, int l_counter)
{
  double min=100000;
  for (int i=0; i<l_counter; i++)
    {
      if (length[i][1]<min)
	min=length[i][1];
    }
  return(min);
}
double regression::find_max(std::vector< std::vector<double> > length, int l_counter)
{
  double max=-100000;
  for (int i=0; i<l_counter; i++)
    {
      if (length[i][1]>max)
	max=length[i][1];
    }
  return(max);
}
