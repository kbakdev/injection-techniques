#pragma once
#include <stdio.h> // for printf
#include <wchar.h>
#include "ntddk.h"

//set of alternative functions doing the same by a different way

PVOID map_buffer_into_process1(HANDLE hProcess, LPBYTE buffer, SIZE_T buffer_size, DWORD protect = PAGE_EXECUTE_READWRITE)
{
    HANDLE hSection = NULL;
    OBJECT_ATTRIBUTES hAttributes;
    memset(&hAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    LARGE_INTEGER maxSize;
    maxSize.HighPart = 0;
    maxSize.LowPart = static_cast<DWORD>(buffer_size);
    NTSTATUS status = NULL;
    
    // Create a new section with specific access rights:
    if ((status = ZwCreateSection( &hSection, SECTION_ALL_ACCESS, NULL, &maxSize, protect, SEC_COMMIT, NULL)) != STATUS_SUCCESS)
    {
        printf("[ERROR] ZwCreateSection failed, status : %x\n", status); // Failed to.
        return NULL;
    }

    PVOID sectionBaseAddress = NULL; // Variable to remote address.
    ULONG viewSize = 0;
    SECTION_INHERIT inheritDisposition = ViewShare; //VIEW_SHARE

    // Map the section in context of current process:
    if ((status = NtMapViewOfSection(hSection, GetCurrentProcess(), &sectionBaseAddress, NULL, NULL, NULL, &viewSize, inheritDisposition, NULL, protect)) != STATUS_SUCCESS)
    {
        printf("[ERROR] NtMapViewOfSection failed, status : %x\n", status);
        return NULL;
    }
    printf("Section BaseAddress: %p\n", sectionBaseAddress);

    // Copy the contents of the buffer to a new section:
    memcpy (sectionBaseAddress, buffer, buffer_size);
    printf("Buffer copied!\n");

    // Map the new section into context of opened process
    PVOID sectionBaseAddress2 = NULL;
    if ((status = NtMapViewOfSection(hSection, hProcess, &sectionBaseAddress2, NULL, NULL, NULL, &viewSize, ViewShare, NULL, protect)) != STATUS_SUCCESS)
    {
        printf("[ERROR] NtMapViewOfSection failed, status : %x\n", status);
        return NULL;
    }

    // Unmap from the context of current process:
    ZwUnmapViewOfSection(GetCurrentProcess(), sectionBaseAddress);
    ZwClose(hSection);

    printf("Section mapped at address: %p\n", sectionBaseAddress2);
    // Return the address where the section was remotely mapped (or NULL, in case of failure):
    return sectionBaseAddress2;
}

LPVOID map_buffer_into_process2(HANDLE hProcess, LPBYTE buffer, SIZE_T buffer_size, DWORD protect = PAGE_EXECUTE_READWRITE)
{
    // Allocate memory space in remote process:
    LPVOID remoteAddress = VirtualAllocEx(hProcess, NULL, buffer_size, MEM_COMMIT | MEM_RESERVE, protect);
    if (remoteAddress == NULL)  {
        // It didn't work, get out.
        printf("Could not allocate memory in the remote process\n");
        return NULL;
    }
    // Write the value of the buffer to the allocated memory
    if (!WriteProcessMemory(hProcess, remoteAddress, buffer, buffer_size, NULL)) {
        VirtualFreeEx(hProcess,remoteAddress, buffer_size, MEM_FREE);
        return NULL;
    }
    return remoteAddress;
}
