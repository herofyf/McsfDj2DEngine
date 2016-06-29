#include "stdafx.h"
#include "dicom_series_attribs_loader_hp_.h"
#include "ace/Thread_Manager.h"
#include "dicom_image_helper.h"

MCSF_DJ2DENGINE_BEGIN_NAMESPACE


unsigned __stdcall LoadImageDicomAttribTask (void* args)
{
	TaskArgs *pTaskArgs = (TaskArgs *)(args);
	if (pTaskArgs == NULL)
		return 0;

	try
	{
		for (ImageSequenceQCIt cit = pTaskArgs->TaskImages.begin(); cit != pTaskArgs->TaskImages.end(); cit ++)
		{
			DicomImageFrameDesc imageFrameDesc;
			ImageFrameSequence sequence = cit->Sequence();
			imageFrameDesc.FilePath(cit->FileName());
			imageFrameDesc.ImageIndex(sequence.ImageIndex());
			imageFrameDesc.DicomFrameIndex(sequence.FrameIndex());
			DicomImageAttrib *pImageDicomAttrib = (DicomImageAttrib *)&imageFrameDesc;
			DicomImageHelper::RetriveImageAttribs(cit->FileName(), pImageDicomAttrib);
			pTaskArgs->Processor->onLoadImageAttribDone(sequence, imageFrameDesc);

			int frameCount = pImageDicomAttrib->GetFrameCount();
			if (frameCount > 1)
			{
				for (int i = 1; i < frameCount; i ++)
				{
					DicomImageFrameDesc *pCloneFrameDesc = imageFrameDesc.Clone();
					if (pCloneFrameDesc)
					{
						pCloneFrameDesc->DicomFrameIndex(i);
						sequence.FrameIndex(i);
						pTaskArgs->Processor->onLoadImageAttribDone(sequence, *pCloneFrameDesc);
						delete pCloneFrameDesc;
					}
				}
			}
		}
	}
	catch(...){}

	return 0;
}

DicomSeriesAttribsLoaderHP::DicomSeriesAttribsLoaderHP(DicomSeriesDescription *pSeriesDesc)
{
	m_pSeriesDesc = pSeriesDesc;
}


DicomSeriesAttribsLoaderHP::~DicomSeriesAttribsLoaderHP(void)
{
}


void DicomSeriesAttribsLoaderHP::LoadImageDicomAttribs(const std::vector<std::string> &image_files, DicomSeriesDescription *pSeriesDesc)
{
#define TASKS_MAX_NUM			20
#define TASKS_MIN_PART_COUNT	120

	int nImageSize = image_files.size();
	if (nImageSize  == 0) 
		return;

	int partition_items_count = TASKS_MIN_PART_COUNT;
	int thrd_count = (nImageSize / partition_items_count) + 1;
	if (thrd_count > TASKS_MAX_NUM)
	{
		thrd_count = TASKS_MAX_NUM;
		partition_items_count = nImageSize / TASKS_MAX_NUM;
	}
	
	DicomSeriesAttribsLoaderHP *Processor = new DicomSeriesAttribsLoaderHP(pSeriesDesc);
	TaskArgs *pWorkGroupsArgs = new TaskArgs[thrd_count];
	HANDLE *pThrdHandles = new HANDLE[thrd_count];

	int index = 0, groupIndex = 0;
	unsigned threadId;

	std::vector<std::string>::const_iterator it;
	DicomImageSequenceArgs arg;
	for(it = image_files.begin(); it != image_files.end(); it ++)
	{
		arg.Set(index, 0, *it);
		
		pWorkGroupsArgs[groupIndex].TaskImages.push_back(arg);
		pWorkGroupsArgs[groupIndex].Processor = Processor;
		
		index ++;
		groupIndex = index / partition_items_count;
	}

	for (int i = 0; i < thrd_count; i ++)
	{
		pThrdHandles[i] = (HANDLE)_beginthreadex(NULL, 0, LoadImageDicomAttribTask, &(pWorkGroupsArgs[i]), 0, &threadId);
	}
	
	for (int i = 0; i < thrd_count; i ++)
	{
		WaitForSingleObject(pThrdHandles[i], INFINITE);
		CloseHandle(pThrdHandles[i]);
	}
	
	delete []pWorkGroupsArgs;
	delete []pThrdHandles;
	
	// to get the result
	Processor->Complete();

	delete Processor;
}

void DicomSeriesAttribsLoaderHP::Complete()
{
	if (m_pSeriesDesc == NULL)
	{
		return;
	}

	m_pSeriesDesc->ResetImagesList();

	std::map<ImageFrameSequence, DicomImageFrameDesc>::const_iterator cit;
	for (cit = loadedResult.begin(); cit != loadedResult.end(); cit ++)
	{
		DicomImageDescription *pDicomImageDesc = new DicomImageDescription(m_pSeriesDesc);
		if (pDicomImageDesc)
		{
			pDicomImageDesc->Set(&(cit->second));
			m_pSeriesDesc->AddDicomImageDescription(pDicomImageDesc);
		}
	}

	loadedResult.clear();
}


MCSF_DJ2DENGINE_END_NAMESPACE